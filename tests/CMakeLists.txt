# 测试配置

# 单元测试
add_executable(test_config_manager
    unit/test_config_manager.cpp
)

target_link_libraries(test_config_manager
    PRIVATE
    dobot_framework
)

add_executable(test_logger
    unit/test_logger.cpp
)

target_link_libraries(test_logger
    PRIVATE
    dobot_framework
)

add_executable(test_motion_types
    unit/test_motion_types.cpp
)

target_link_libraries(test_motion_types
    PRIVATE
    dobot_framework
)

# 集成测试
add_executable(test_robot_integration
    integration/test_robot_integration.cpp
)

target_link_libraries(test_robot_integration
    PRIVATE
    dobot_framework
)

add_executable(test_motion_controller
    integration/test_motion_controller.cpp
)

target_link_libraries(test_motion_controller
    PRIVATE
    dobot_framework
)

# 设置测试输出目录
set_target_properties(
    test_config_manager
    test_logger
    test_motion_types
    test_robot_integration
    test_motion_controller
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
)

# 添加测试
add_test(NAME test_config_manager COMMAND test_config_manager)
add_test(NAME test_logger COMMAND test_logger)
add_test(NAME test_motion_types COMMAND test_motion_types)
add_test(NAME test_robot_integration COMMAND test_robot_integration)
add_test(NAME test_motion_controller COMMAND test_motion_controller)

# 安装测试
option(INSTALL_TESTS "Install tests" OFF)
if(INSTALL_TESTS)
    install(TARGETS
        test_config_manager
        test_logger
        test_motion_types
        test_robot_integration
        test_motion_controller
        RUNTIME DESTINATION bin/tests
    )
endif()

# 测试数据
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data)
configure_file(
    ${CMAKE_SOURCE_DIR}/config/robot/default_robot_config.json
    ${CMAKE_BINARY_DIR}/test_data/default_robot_config.json
    COPYONLY
)

# 内存检查（需要valgrind）
find_program(MEMORYCHECK_COMMAND valgrind)
if(MEMORYCHECK_COMMAND)
    set(MEMORYCHECK_COMMAND_OPTIONS
        "--tool=memcheck --leak-check=full --show-reachable=yes --num-callers=20"
    )
endif()

# 测试覆盖率（需要gcov）
option(ENABLE_COVERAGE "Enable test coverage" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(dobot_framework PRIVATE --coverage)
        target_link_libraries(dobot_framework PRIVATE -lgcov)
    else()
        message(WARNING "Coverage not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()