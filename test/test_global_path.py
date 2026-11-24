import argparse
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time

import rclpy
from action_msgs.msg import GoalStatus, GoalStatusArray
from geometry_msgs.msg import PoseStamped, PoseWithCovarianceStamped, Twist
from nav_msgs.msg import Path as PathMsg
from rclpy.qos import DurabilityPolicy, QoSProfile


def run(args, directory):
    map_file = directory / "ramp.bin"
    subprocess.run([args.map_generator, str(map_file)], check=True)
    config = directory / "params.yaml"
    config.write_text(Path(args.config).read_text()
                      .replace("$(var map_file)", str(map_file))
                      .replace("$(var map_resolution)", "0.1"))
    log_file = directory / "demo.log"
    with log_file.open("w") as log:
        process = subprocess.Popen([args.executable, "--ros-args", "--params-file", str(config)],
                                   stdout=log, stderr=subprocess.STDOUT)
        rclpy.init()
        node = rclpy.create_node("global_path_demo_test")
        paths = []
        commands = []
        statuses = []
        subscriptions = [
            node.create_subscription(PathMsg, "/global_path", paths.append,
                                     QoSProfile(depth=10, durability=DurabilityPolicy.TRANSIENT_LOCAL)),
            node.create_subscription(Twist, "/cmd_vel", commands.append, 10),
            node.create_subscription(GoalStatusArray, "/finenav_demo/navigate_to_pose/_action/status",
                                     statuses.append,
                                     QoSProfile(depth=10, durability=DurabilityPolicy.TRANSIENT_LOCAL)),
        ]
        goals = node.create_publisher(PoseStamped, "/goal_pose", 10)
        localization = node.create_publisher(PoseWithCovarianceStamped, "/localization_pose", 10)

        def spin_until(condition, timeout=10.0, publish_pose=False):
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                assert process.poll() is None, "Demo exited before producing a path"
                if publish_pose:
                    pose = PoseWithCovarianceStamped()
                    pose.header.frame_id = "map"
                    pose.header.stamp = node.get_clock().now().to_msg()
                    pose.pose.pose.position.x = 0.05
                    pose.pose.pose.position.y = 0.05
                    pose.pose.pose.position.z = 1.0125
                    pose.pose.pose.orientation.w = 1.0
                    localization.publish(pose)
                rclpy.spin_once(node, timeout_sec=0.05)
                if condition():
                    return True
            return False

        def send_goal(x, y, z):
            goal = PoseStamped()
            goal.header.frame_id = "map"
            goal.header.stamp = node.get_clock().now().to_msg()
            goal.pose.position.x, goal.pose.position.y, goal.pose.position.z = x, y, z
            goal.pose.orientation.w = 1.0
            goals.publish(goal)

        def check_path(path, x, y, z):
            assert path.header.frame_id == "map"
            assert all(p.header.frame_id == "map" for p in path.poses)
            endpoint = path.poses[-1].pose.position
            assert abs(endpoint.x - x) <= 0.2, (endpoint, x)
            assert abs(endpoint.y - y) <= 0.2, (endpoint, y)
            assert abs(endpoint.z - z) <= 0.1, (endpoint, z)
            heights = [p.pose.position.z for p in path.poses]
            assert max(heights) - min(heights) > 0.1, heights

        def request_count(status):
            return len({bytes(entry.goal_info.goal_id.uuid)
                        for message in statuses for entry in message.status_list
                        if entry.status == status})

        try:
            assert spin_until(lambda: goals.get_subscription_count() > 0 and
                              localization.get_subscription_count() > 0), "Inputs not connected"
            send_goal(1.05, 0.05, 1.2625)
            spin_until(lambda: False, timeout=0.5)
            assert not any(p.poses for p in paths), "Planned without localization"
            assert spin_until(lambda: any(p.poses for p in paths), publish_pose=True), "No first path"
            check_path(next(p for p in reversed(paths) if p.poses), 1.05, 0.05, 1.2625)
            assert spin_until(lambda: request_count(GoalStatus.STATUS_SUCCEEDED) == 1), "First request did not finish"

            paths.clear()
            send_goal(-1.05, 0.55, 0.7375)
            assert spin_until(lambda: any(p.poses for p in paths), publish_pose=True), "No second path"
            check_path(next(p for p in reversed(paths) if p.poses), -1.05, 0.55, 0.7375)
            assert spin_until(lambda: request_count(GoalStatus.STATUS_SUCCEEDED) == 2), "Second request did not finish"

            paths.clear()
            send_goal(100.0, 0.0, 1.0)
            assert spin_until(lambda: bool(paths), publish_pose=True), "Old path was not cleared"
            assert spin_until(lambda: request_count(GoalStatus.STATUS_ABORTED) == 1,
                              publish_pose=True), "Unreachable goal did not fail"
            assert not any(p.poses for p in paths), "Published a path for an unreachable goal"
            assert not commands, "Global planner published velocity commands"
            print("PASS: offline NDT load, localization wait, two 3D goals, unreachable goal, no cmd_vel")
        except BaseException:
            print(log_file.read_text())
            raise
        finally:
            for subscription in subscriptions:
                node.destroy_subscription(subscription)
            node.destroy_node()
            rclpy.shutdown()
            if process.poll() is None:
                process.send_signal(signal.SIGINT)
                try:
                    process.wait(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", required=True)
    parser.add_argument("--map-generator", required=True)
    parser.add_argument("--config", required=True)
    arguments = parser.parse_args()
    os.environ["ROS_DOMAIN_ID"] = str(100 + os.getpid() % 80)
    os.environ["ROS_LOCALHOST_ONLY"] = "1"
    with tempfile.TemporaryDirectory(prefix="finenav_demo_test_") as temporary:
        run(arguments, Path(temporary))
