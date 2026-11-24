#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>

#include "finenav_navigator/chassis_command/i_chassis_command.hpp"
#include "finenav_navigator/pipeline_controller.hpp"

namespace finenav::demo {

class ComputeGlobalPath final : public BT::StatefulActionNode {
public:
    using BT::StatefulActionNode::StatefulActionNode;
    using Feedback = finenav_interfaces::msg::TaskFeedback;

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<navigator::PipelineController*>("pipeline", "{@_pipeline}", "Pipeline"),
            BT::InputPort<rclcpp::Node::SharedPtr>("node", "{@_node}", "Application node"),
            BT::InputPort<navigator::IChassisCommand*>("output", "{@_chassis_cmd}", "Path output"),
            BT::InputPort<std::vector<geometry_msgs::msg::Pose>>(
                "goals", "{@goal_poses}", "Requested goal"),
            BT::InputPort<double>("timeout", 15.0, "Planning timeout in seconds"),
        };
    }

    BT::NodeStatus onStart() override {
        std::vector<geometry_msgs::msg::Pose> goals;
        double timeout = 0.0;
        navigator::IChassisCommand* output = nullptr;
        if (!getInput("pipeline", pipeline_) || !pipeline_ ||
            !getInput("node", node_) || !node_ ||
            !getInput("output", output) || !output ||
            !getInput("goals", goals) || goals.empty() ||
            !getInput("timeout", timeout) || !std::isfinite(timeout) || timeout <= 0.0 ||
            !pipeline_->hasBranch("global_plan")) {
            return BT::NodeStatus::FAILURE;
        }
        output->stop();
        for (const auto& goal : goals) {
            if (!std::isfinite(goal.position.x) || !std::isfinite(goal.position.y) ||
                !std::isfinite(goal.position.z)) {
                RCLCPP_ERROR(node_->get_logger(), "Goal position must be finite");
                return BT::NodeStatus::FAILURE;
            }
        }
        previous_ = pipeline_->feedback("global_planner");
        deadline_ = std::chrono::steady_clock::now() +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(timeout));
        pipeline_->publishNavigationGoal(std::move(goals));
        pipeline_->activateBranch("global_plan");
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        const auto feedback = pipeline_->feedback("global_planner");
        if (feedback && feedback != previous_) {
            if (feedback->status == Feedback::FAULTED) {
                RCLCPP_ERROR(node_->get_logger(), "Global planning failed");
                return BT::NodeStatus::FAILURE;
            }
            if (feedback->status == Feedback::RUNNING &&
                (feedback->trajectory.valid_fields & finenav_interfaces::msg::Trajectory::TRAJ_POSE)) {
                if (feedback->trajectory.poses.empty()) {
                    RCLCPP_ERROR(node_->get_logger(), "Planner returned an empty path");
                    return BT::NodeStatus::FAILURE;
                }
                return BT::NodeStatus::SUCCESS;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline_) {
            RCLCPP_ERROR(node_->get_logger(), "Global planning timed out; check /localization_pose");
            return BT::NodeStatus::FAILURE;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        if (pipeline_ && pipeline_->isActive("global_plan")) pipeline_->stopAll();
    }

private:
    navigator::PipelineController* pipeline_ = nullptr;
    rclcpp::Node::SharedPtr node_;
    std::shared_ptr<const Feedback> previous_;
    std::chrono::steady_clock::time_point deadline_;
};

}  // namespace finenav::demo

BT_REGISTER_NODES(factory) {
    factory.registerNodeType<finenav::demo::ComputeGlobalPath>("ComputeGlobalPath");
}
