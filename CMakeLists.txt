cmake_minimum_required(VERSION 3.16)
project(dobot_atom_sdk_framework VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -fPIC)
endif()

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找必要的包
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# 查找Eigen3
if(NOT TARGET Eigen3::eigen)
    find_package(Eigen3 3.3 REQUIRED)
endif()

# 查找YAML-CPP
if(NOT TARGET yaml-cpp)
    pkg_check_modules(YAML_CPP yaml-cpp)
    if(YAML_CPP_FOUND)
        add_library(yaml-cpp INTERFACE IMPORTED)
        set_target_properties(yaml-cpp PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${YAML_CPP_INCLUDE_DIRS}
            INTERFACE_LINK_LIBRARIES ${YAML_CPP_LIBRARIES})
    else()
        message(FATAL_ERROR "YAML-CPP not found. Please install it.")
    endif()
endif()

# 查找CycloneDDS
if(NOT TARGET cyclonedds)
    find_path(CYCLONEDDS_INCLUDE_DIR
        NAMES dds/ddsc/dds_public.h
        PATHS /usr/include /usr/local/include
        NO_DEFAULT_PATH)

    find_library(CYCLONEDDS_LIBRARY
        NAMES ddsc
        PATHS /usr/lib /usr/local/lib /lib
        NO_DEFAULT_PATH)

    if(CYCLONEDDS_INCLUDE_DIR AND CYCLONEDDS_LIBRARY)
        add_library(cyclonedds INTERFACE IMPORTED)
        set_target_properties(cyclonedds PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${CYCLONEDDS_INCLUDE_DIR}
            INTERFACE_LINK_LIBRARIES ${CYCLONEDDS_LIBRARY})
    else()
        message(WARNING "CycloneDDS not found. DDS functionality will be disabled.")
    endif()
endif()

# 编译定义
add_compile_definitions(_USE_MATH_DEFINES)

# 头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${EIGEN3_INCLUDE_DIR}
    ${CYCLONEDDS_INCLUDE_DIR}
)

# 链接库
link_libraries(
    Threads::Threads
    Eigen3::eigen
    yaml-cpp
    cyclonedds
)

# 创建Framework库
set(FRAMEWORK_SOURCES
    src/robot/dobot_robot.cpp
    src/utils/config_manager.cpp
    src/utils/logger.cpp
    src/monitor/state_monitor.cpp
    src/monitor/data_recorder.cpp
    src/control/motion_controller.cpp
    src/safety/safety_manager.cpp
    src/safety/emergency_stop.cpp
)

add_library(dobot_framework STATIC ${FRAMEWORK_SOURCES})

# 设置框架库的属性
target_compile_features(dobot_framework PUBLIC cxx_std_17)
target_link_libraries(dobot_framework
    Threads::Threads
    Eigen3::eigen
    yaml-cpp
    cyclonedds
)

# 包含目录
target_include_directories(dobot_framework PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 示例程序
add_subdirectory(examples)

# 测试
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装配置
install(DIRECTORY include/ DESTINATION include)
install(TARGETS dobot_framework
    EXPORT DobotFrameworkTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# 导出配置
install(EXPORT DobotFrameworkTargets
    FILE DobotFrameworkTargets.cmake
    NAMESPACE Dobot::
    DESTINATION lib/cmake/DobotFramework
)

# 创建配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    DobotFrameworkConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/DobotFrameworkConfigVersion.cmake
    DESTINATION lib/cmake/DobotFramework
)

# 打包规则
include(CPack)
set(CPACK_PACKAGE_NAME "DobotAtomSDKFramework")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Dobot Atom SDK Framework")
set(CPACK_PACKAGE_VENDOR "Your Company")
set(CPACK_PACKAGE_CONTACT "contact@example.com")

# 打包类型
include(CPackComponent)
set(CPACK_COMPONENT_FRAMEWORK_DISPLAY_NAME "Core Framework")
set(CPACK_COMPONENT_FRAMEWORK_DESCRIPTION "Core framework library")

set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "Examples")
set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "Example programs")

set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Development Headers")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "Header files for development")

# 打包选项
set(CPACK_COMPONENTS_ALL framework examples headers)

# 平台特定设置
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

# 文档
option(BUILD_DOCUMENTATION "Build documentation" OFF)
if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen found, generating documentation...")

        # 创建Doxygen配置文件
        configure_file(
            ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in
            ${CMAKE_BINARY_DIR}/Doxyfile
            @ONLY
        )

        # 添加文档目标
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating documentation with Doxygen"
            VERBATIM
        )

        # 安装文档
        install(DIRECTORY ${CMAKE_BINARY_DIR}/html/
            DESTINATION share/doc/dobot_framework
        )
    endif()
endif()

# 调试选项
option(ENABLE_DEBUG "Enable debug features" OFF)
if(ENABLE_DEBUG)
    add_definitions(-DDEBUG)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 静态分析选项
option(ENABLE_STATIC_ANALYSIS "Enable static analysis" OFF)
if(ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY};-extra-arg=-Wno-unknown-warning-option")
    else()
        message(WARNING "clang-tidy not found, static analysis disabled")
    endif()
endif()