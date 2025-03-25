import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch.actions import ExecuteProcess
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

def generate_launch_description():

    use_sim_time = LaunchConfiguration('use_sim_time', default='false')

    urdf_file_name = 'model.urdf'
    urdf = os.path.join(
        get_package_share_directory('hexapod_description'),
        urdf_file_name)
    with open(urdf, 'r') as infp:
        robot_desc = infp.read()

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use simulation (Gazebo) clock if true'),
        
        IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            get_package_share_directory('gazebo_ros'), 'launch'), '/gazebo.launch.py']),
        ),
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{'use_sim_time': use_sim_time, 'robot_description': robot_desc}],
            arguments=[urdf]),

        Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=["/robot_description", os.path.join(get_package_share_directory('hexapod_description'), 'config', 'hexapod_control.yaml')],
        output={
            "stdout": "screen",
            "stderr": "screen",
        }
    ),

    Node(
        name="joint_state_broadcaster",
        package="controller_manager",
        executable="spawner",
        output="screen",
        arguments=[
            "joint_state_broadcaster",
            "--controller-manager",
            "/controller_manager"       
        ]
    ),
    Node(
        name="joint_controller",
        package="controller_manager",
        executable="spawner",
        output="screen",
        arguments=[
            "joint_controller",
            "--controller-manager",
            "/controller_manager"       
        ]
    ),
    Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', os.path.join(get_package_share_directory('hexapod_description'), 'rviz', 'model.rviz')]
    ),
    Node(package='gazebo_ros', executable='spawn_entity.py',
                arguments=['-topic', 'robot_description',
                            '-entity', 'hexapod'],
                output='screen')

    ])