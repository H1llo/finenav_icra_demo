#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

#include <nav_msgs/msg/path.hpp>
#include <rclcpp/rclcpp.hpp>

#include "finenav_engine_cpp/finenav_engine.hpp"
#include "finenav_navigator/chassis_command/i_chassis_command.hpp"

namespace finenav::demo {

// Use the Engine's trajectory output channel to publish a path.
class GlobalPathOutput final : public navigator::IChassisCommand {
public:
    explicit GlobalPathOutput(rclcpp::Node::SharedPtr node) : node_(std::move(node)) {
        publisher_ = node_->create_publisher<nav_msgs::msg::Path>(
            "/global_path", rclcpp::QoS(1).transient_local());
    }

    bool validate(const finenav_interfaces::msg::Trajectory& trajectory) const override {
        return (trajectory.valid_fields & finenav_interfaces::msg::Trajectory::TRAJ_POSE) &&
            !trajectory.poses.empty() &&
            std::all_of(trajectory.poses.begin(), trajectory.poses.end(), [](const auto& pose) {
                return std::isfinite(pose.position.x) && std::isfinite(pose.position.y) &&
                       std::isfinite(pose.position.z);
            });
    }

    void publish(const finenav_interfaces::msg::Trajectory& trajectory) override {
        nav_msgs::msg::Path path;
        path.header.frame_id = common::frames().world_fixed;
        path.header.stamp = node_->now();
        path.poses.reserve(trajectory.poses.size());
        for (const auto& pose : trajectory.poses) {
            geometry_msgs::msg::PoseStamped waypoint;
            waypoint.header = path.header;
            waypoint.pose = pose;
            path.poses.push_back(waypoint);
        }
        publisher_->publish(path);
        RCLCPP_INFO(node_->get_logger(), "Published /global_path: %zu waypoints", path.poses.size());
    }

    // Clear the latched result when a new planning request begins.
    void stop() override {
        nav_msgs::msg::Path path;
        path.header.frame_id = common::frames().world_fixed;
        path.header.stamp = node_->now();
        publisher_->publish(path);
    }

private:
    rclcpp::Node::SharedPtr node_;
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr publisher_;
};

}  // namespace finenav::demo

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("finenav_demo");
    int result = 0;
    try {
        auto engine = std::make_shared<finenav::FineNavEngine>(node);
        engine->setChassisCommand(std::make_shared<finenav::demo::GlobalPathOutput>(node));
        engine->setDefaultBehaviorTree("ComputeGlobalPathDemo");
        if (!engine->loadAlgoPlugins({"finenav_ndt_hierarchical_map",
                                     "finenav_tomography_planner", "tomography_ndt_view"}) ||
            !engine->createControlLayer("tomography_ndt_view", "global_planner",
                                       finenav::core::EventDrivenPolicy{}) ||
            !engine->registerControlBranch("global_plan", {"global_planner"})) {
            throw std::runtime_error(
                "Cannot assemble global planner; build tomography_ndt_view with "
                "-DTOMOGRAPHY_NDT_WITH_FINENAV=ON and source the workspace.");
        }
        RCLCPP_INFO(node->get_logger(),
                    "Ready: /localization_pose + /goal_pose -> /global_path (map frame)");
        rclcpp::spin(node);
    } catch (const std::exception& error) {
        RCLCPP_ERROR(node->get_logger(), "%s", error.what());
        result = 1;
    }
    rclcpp::shutdown();
    return result;
}
