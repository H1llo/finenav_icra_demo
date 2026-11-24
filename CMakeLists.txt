cmake_minimum_required(VERSION 3.22)
project(finenav_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(finenav_engine_cpp REQUIRED)
find_package(finenav_navigator REQUIRED)
find_package(finenav_interfaces REQUIRED)

add_executable(finenav_demo src/finenav_demo.cpp)
ament_target_dependencies(finenav_demo
  rclcpp nav_msgs geometry_msgs finenav_engine_cpp finenav_navigator finenav_interfaces)

add_library(compute_global_path SHARED src/compute_global_path.cpp)
target_compile_definitions(compute_global_path PRIVATE BT_PLUGIN_EXPORT)
ament_target_dependencies(compute_global_path
  rclcpp behaviortree_cpp finenav_navigator finenav_interfaces)

install(TARGETS finenav_demo DESTINATION lib/${PROJECT_NAME})
install(TARGETS compute_global_path LIBRARY DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY behavior_trees config launch DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_test REQUIRED)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  find_package(finenav_ndt_octomap REQUIRED)
  add_executable(create_demo_test_map test/create_test_map.cpp)
  ament_target_dependencies(create_demo_test_map finenav_ndt_octomap)
  ament_add_test(global_path_demo
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test/test_global_path.py"
      --executable "$<TARGET_FILE:finenav_demo>"
      --map-generator "$<TARGET_FILE:create_demo_test_map>"
      --config "${CMAKE_CURRENT_SOURCE_DIR}/config/finenav_demo.yaml"
    TIMEOUT 60
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO)
endif()

ament_package()
