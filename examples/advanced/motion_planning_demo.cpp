/**
 * @file motion_planning_demo.cpp
 * @brief 高级示例：运动规划与执行
 * @author Framework
 * @date 2024
 */

#include "../src/robot/dobot_robot.h"
#include "../src/control/motion_controller.h"
#include "../src/control/motion_types.h"
#include "../src/utils/logger.h"
#include "../src/utils/config_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace robot_framework;

/**
 * @brief 运动规划演示程序
 */
int main() {
    // 初始化
    Logger::getInstance().setLevel(LogLevel::DEBUG);
    LOG_INFO("=== Dobot Atom SDK Framework 运动规划示例 ===");

    try {
        // 1. 创建并初始化机器人
        auto robot = RobotFactory::createRobot();
        robot->connect("192.168.8.234", 50051);

        // 2. 创建运动控制器
        MotionController controller;
        MotionControllerConfig config;
        config.robot_ip = "192.168.8.234";
        config.robot_port = 50051;
        config.control_frequency = 100.0;
        config.enable_safety_checks = true;

        if (!controller.initialize(config)) {
            LOG_ERROR("运动控制器初始化失败");
            return 1;
        }

        // 3. 设置回调函数
        controller.setMotionCallback([](const MotionResult& result) {
            const char* status_strings[] = {
                "SUCCESS", "FAILURE", "TIMEOUT", "ABORTED", "ERROR"
            };
            LOG_INFO("运动完成 - 状态: %s", status_strings[(int)result.status]);
            if (!result.message.empty()) {
                LOG_INFO("消息: %s", result.message.c_str());
            }
        });

        // 4. 设置运动规划参数
        MotionPlanningParams planning_params;
        planning_params.mode = MotionMode::JOINT;
        planning_params.max_velocity = 1.0;      // rad/s
        planning_params.max_acceleration = 2.0;    // rad/s^2
        planning_params.enable_smoothing = true;
        planning_params.smoothing_time = 0.1;

        // 5. 创建轨迹生成器
        auto trajectory_generator = std::make_shared<SimpleTrajectoryGenerator>();
        controller.setTrajectoryGenerator(trajectory_generator);

        // 6. 定义几个关键点
        std::vector<JointPoint> waypoints;

        // 起始位置
        JointPoint start;
        start.positions = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        start.velocities = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        start.duration = 0.0;
        start.description = "起始位置";
        waypoints.push_back(start);

        // 中间位置1
        JointPoint waypoint1;
        waypoint1.positions = {0.5, 0.3, -0.8, 0.2, 0.0, 0.0};
        waypoint1.velocities = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        waypoint1.duration = 2.0;
        waypoint1.description = "中间点1";
        waypoints.push_back(waypoint1);

        // 中间位置2
        JointPoint waypoint2;
        waypoint2.positions = {-0.3, 0.5, -1.0, -0.2, 0.1, 0.0};
        waypoint2.velocities = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        waypoint2.duration = 2.0;
        waypoint2.description = "中间点2";
        waypoints.push_back(waypoint2);

        // 目标位置
        JointPoint goal;
        goal.positions = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        goal.velocities = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        goal.duration = 0.0;
        goal.description = "目标位置";
        waypoints.push_back(goal);

        // 7. 生成轨迹
        Trajectory trajectory;
        trajectory.name = "演示轨迹";
        trajectory.description = "通过多个关键点的运动演示";

        if (!trajectory_generator->generateTrajectory(waypoints, trajectory, planning_params)) {
            LOG_ERROR("轨迹生成失败");
            return 1;
        }

        LOG_INFO("轨迹生成成功，共 %d 个点，总时长 %.2f 秒",
                trajectory.point_count, trajectory.total_duration);

        // 8. 执行轨迹
        LOG_INFO("开始执行轨迹...");
        MotionCommand command;
        command.type = MotionCommand::Type::EXECUTE_TRAJECTORY;
        command.trajectory = trajectory;
        command.wait_for_completion = true;
        command.timeout = 30.0;

        auto future = controller.executeCommand(command);

        // 9. 监控执行进度
        while (future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
            auto current_state = controller.getState();
            LOG_DEBUG("控制器状态: %d", (int)current_state);

            // 可以在这里添加实时状态监控
            // auto joint_state = controller.getCurrentJointState();
            // LOG_DEBUG("当前关节位置: [%.2f, %.2f, ...]",
            //          joint_state.positions[0], joint_state.positions[1]);
        }

        // 10. 获取执行结果
        MotionResult result = future.get();
        LOG_INFO("轨迹执行完成，状态: %d", (int)result.status);

        // 11. 笛卡尔空间运动示例
        LOG_INFO("=== 笛卡尔空间运动示例 ===");

        MotionCommand cartesian_cmd;
        cartesian_cmd.type = MotionCommand::Type::MOVE_CARTESIAN;

        CartesianPoint target;
        target.position = {0.3, 0.0, 0.5};  // x=0.3m, y=0.0m, z=0.5m
        target.orientation = {0.0, 0.0, 0.0}; // 欧拉角
        target.duration = 3.0;
        target.description = "笛卡尔目标位置";

        cartesian_cmd.cartesian_point = target;
        cartesian_cmd.wait_for_completion = true;
        cartesian_cmd.timeout = 10.0;

        auto cartesian_future = controller.executeCommand(cartesian_cmd);
        cartesian_future.wait();

        MotionResult cartesian_result = cartesian_future.get();
        LOG_INFO("笛卡尔运动完成，状态: %d", (int)cartesian_result.status);

        // 12. 速度控制示例
        LOG_INFO("=== 速度控制示例 ===");

        MotionCommand velocity_cmd;
        velocity_cmd.type = MotionCommand::Type::VELOCITY_CMD;

        velocity_cmd.velocity_cmd.linear_vel = {0.1, 0.0, 0.0};  // 0.1 m/s 向前
        velocity_cmd.velocity_cmd.angular_vel = {0.0, 0.0, 0.0}; // 无旋转
        velocity_cmd.velocity_cmd.duration = 2.0;               // 持续2秒
        velocity_cmd.wait_for_completion = true;

        auto velocity_future = controller.executeCommand(velocity_cmd);
        velocity_future.wait();

        LOG_INFO("速度控制完成");

        // 13. 清理
        LOG_INFO("断开连接...");
        robot->disconnect();

    } catch (const std::exception& e) {
        LOG_ERROR("异常: %s", e.what());
        return 1;
    }

    LOG_INFO("运动规划演示完成");
    return 0;
}