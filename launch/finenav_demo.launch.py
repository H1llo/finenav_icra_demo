from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterFile, ParameterValue


def generate_launch_description():
    package_dir = Path(get_package_share_directory("finenav_demo"))
    return LaunchDescription([
        DeclareLaunchArgument("map_file",
                              default_value="/home/h1llo/Downloads/wheelchair.ot",
                              description="IGOT v1 NDT map path"),
        DeclareLaunchArgument("map_resolution", default_value="0.05",
                              description="Local grid resolution; must match the NDT file"),
        DeclareLaunchArgument("use_sim_time", default_value="false",
                              description="Use the /clock simulation time"),
        DeclareLaunchArgument("params_file",
                              default_value=str(package_dir / "config" / "finenav_demo.yaml"),
                              description="ROS 2 parameters for the demo and Engine components"),
        Node(
            package="finenav_demo",
            executable="finenav_demo",
            output="screen",
            parameters=[
                ParameterFile(LaunchConfiguration("params_file"), allow_substs=True),
                {"use_sim_time": ParameterValue(LaunchConfiguration("use_sim_time"), value_type=bool)},
            ],
        ),
    ])
