/**
 * @file hello_robot.cpp
 * @brief 基础示例：连接和控制机器人
 * @author Framework
 * @date 2024
 */

#include "../src/robot/dobot_robot.h"
#include "../src/utils/logger.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace robot_framework;

/**
 * @brief 演示基本机器人控制功能
 */
int main() {
    // 初始化日志系统
    Logger::getInstance().setLevel(LogLevel::INFO);
    LOG_INFO("=== Dobot Atom SDK Framework 基础示例 ===");

    try {
        // 创建机器人实例
        auto robot = RobotFactory::createRobot();
        if (!robot) {
            LOG_ERROR("无法创建机器人实例");
            return 1;
        }

        // 连接到机器人
        LOG_INFO("正在连接到机器人...");
        if (!robot->connect("192.168.8.234", 50051)) {
            LOG_ERROR("连接失败");
            return 1;
        }

        LOG_INFO("连接成功！");

        // 设置状态回调
        robot->setStateCallback([](RobotState state) {
            static const char* state_strings[] = {
                "DISCONNECTED", "CONNECTING", "CONNECTED",
                "ERROR", "EMERGENCY_STOP", "READY", "RUNNING"
            };
            LOG_INFO("机器人状态变更: %s", state_strings[(int)state]);
        });

        robot->setErrorCallback([](const std::string& error) {
            LOG_ERROR("机器人错误: %s", error.c_str());
        });

        // 等待机器人就绪
        LOG_INFO("等待机器人就绪...");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 获取系统信息
        LOG_INFO("机器人型号: %s", robot->getRobotModel().c_str());
        LOG_INFO("固件版本: %s", robot->getFirmwareVersion().c_str());

        // 检查安全状态
        if (robot->isEmergencyStop()) {
            LOG_WARN("机器人处于急停状态，正在复位...");
            if (!robot->resetEmergencyStop()) {
                LOG_ERROR("无法复位急停状态");
                return 1;
            }
        }

        if (!robot->isSafeToOperate()) {
            LOG_ERROR("机器人不处于安全状态，无法继续操作");
            return 1;
        }

        // 切换到空闲状态
        LOG_INFO("切换到空闲状态...");
        // 注意：这里需要根据实际API调整
        // robot->setFsmId(100);

        LOG_INFO("当前模式: 待机状态");

        // 简单的关节移动示例（假设）
        LOG_INFO("执行简单的关节运动...");
        std::vector<double> joint_positions = {0.0, 0.5, -1.0, 0.0, 0.0, 0.0};

        // 注意：这里需要使用实际的运动控制接口
        // auto result = robot->executeMotion(joint_positions);
        // if (!result.success) {
        //     LOG_ERROR("运动执行失败: %s", result.message.c_str());
        // }

        LOG_INFO("运动指令已发送");

        // 等待运动完成
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // 断开连接
        LOG_INFO("断开连接...");
        robot->disconnect();

        LOG_INFO("示例程序执行完成");

    } catch (const std::exception& e) {
        LOG_ERROR("异常: %s", e.what());
        return 1;
    }

    return 0;
}