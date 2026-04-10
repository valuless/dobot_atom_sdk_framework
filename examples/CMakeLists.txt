# 示例程序配置

# 基础示例
add_executable(hello_robot
    basic/hello_robot.cpp
)

# 链接框架库
target_link_libraries(hello_robot
    PRIVATE
    dobot_framework
)

# 设置输出目录
set_target_properties(hello_robot PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
)

# 高级示例
add_executable(motion_planning_demo
    advanced/motion_planning_demo.cpp
)

target_link_libraries(motion_planning_demo
    PRIVATE
    dobot_framework
)

set_target_properties(motion_planning_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
)

add_executable(safety_monitoring_demo
    advanced/safety_monitoring_demo.cpp
)

target_link_libraries(safety_monitoring_demo
    PRIVATE
    dobot_framework
)

set_target_properties(safety_monitoring_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
)

# 安装示例程序
install(TARGETS hello_robot
    RUNTIME DESTINATION bin/examples
)

install(TARGETS motion_planning_demo
    RUNTIME DESTINATION bin/examples
)

install(TARGETS safety_monitoring_demo
    RUNTIME DESTINATION bin/examples
)

# 复制配置文件到示例目录
configure_file(
    ${CMAKE_SOURCE_DIR}/config/robot/default_robot_config.json
    ${CMAKE_BINARY_DIR}/bin/examples/config/default_robot_config.json
    COPYONLY
)

install(FILES
    ${CMAKE_SOURCE_DIR}/config/robot/default_robot_config.json
    DESTINATION bin/examples/config
)