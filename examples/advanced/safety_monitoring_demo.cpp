/**
 * @file safety_monitoring_demo.cpp
 * @brief 高级示例：安全监控与急停管理
 * @author Framework
 * @date 2024
 */

#include "../src/robot/dobot_robot.h"
#include "../src/monitor/state_monitor.h"
#include "../src/safety/safety_manager.h"
#include "../src/safety/emergency_stop.h"
#include "../src/utils/logger.h"
#include "../src/utils/config_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

using namespace robot_framework;

/**
 * @brief 安全监控演示程序
 */
int main() {
    // 初始化
    Logger::getInstance().setLevel(LogLevel::DEBUG);
    LOG_INFO("=== Dobot Atom SDK Framework 安全监控示例 ===");

    try {
        // 1. 创建机器人实例
        auto robot = RobotFactory::createRobot();
        robot->connect("192.168.8.234", 50051);

        // 2. 创建状态监控器
        StateMonitor monitor(robot.get());
        MonitorConfig monitor_config;
        monitor_config.update_frequency = 50.0;  // 50Hz
        monitor_config.enable_joint_monitoring = true;
        monitor_config.enable_system_monitoring = true;
        monitor_config.joint_temperature_threshold = 60.0;
        monitor_config.enable_logging = true;

        monitor.setConfig(monitor_config);

        // 设置状态更新回调
        monitor.setStateCallback([](const RobotStateData& state) {
            LOG_DEBUG("状态更新 - FSM ID: %d", state.fsm_id);

            // 检查关键关节温度
            for (const auto& [name, joint] : state.joints) {
                if (joint.temperature > 50.0) {
                    LOG_WARN("关节 %s 温度偏高: %.1f°C", name.c_str(), joint.temperature);
                }
            }
        });

        // 3. 创建急停管理器
        auto estop_manager = std::make_shared<EmergencyStopManager>();
        EStopConfig estop_config;
        estop_config.auto_reset_after = false;
        estop_config.enable_notifications = true;
        estop_config.enable_detailed_logging = true;

        estop_manager->initialize(estop_config);

        // 设置急停状态回调
        estop_manager->setStateCallback([](const EStopEvent& event) {
            LOG_INFO("急停状态变更: %s -> %s (来源: %d)",
                    event.old_state == EStopState::INACTIVE ? "INACTIVE" : "ACTIVE",
                    event.new_state == EStopState::ACTIVE ? "ACTIVE" : "INACTIVE",
                    (int)event.source);
        });

        // 4. 创建安全管理器
        auto safety_manager = SafetyManagerFactory::createComprehensiveSafetyManager(&monitor);
        SafetyManagerConfig safety_config;
        safety_config.monitoring_frequency = 100.0;
        safety_config.enable_all_rules = true;
        safety_config.enable_auto_recovery = false;

        safety_manager->initialize(safety_config);

        // 设置安全事件回调
        safety_manager->setEventCallback([](const SafetyEvent& event) {
            LOG_INFO("安全事件: %s (严重级别: %d)",
                    event.message.c_str(), (int)event.severity);
        });

        // 5. 启动所有监控系统
        LOG_INFO("启动监控线程...");
        if (!monitor.start()) {
            LOG_ERROR("无法启动状态监控器");
            return 1;
        }

        if (!safety_manager->start()) {
            LOG_ERROR("无法启动安全管理器");
            return 1;
        }

        if (!estop_manager->start()) {
            LOG_ERROR("无法启动急停管理器");
            return 1;
        }

        // 6. 运行监控演示
        LOG_INFO("开始安全监控演示（持续30秒）...");

        for (int i = 0; i < 30; ++i) {
            LOG_INFO("--- 第 %d 秒 ---", i + 1);

            // 模拟一些事件（实际应用中不需要）
            if (i == 10) {
                LOG_WARN("模拟温度警告...");
                // 这里会自动触发温度规则检查
            }

            if (i == 20) {
                LOG_WARN("模拟急停按钮按下...");
                // 模拟急停
                estop_manager->activate(EStopSource::HARDWARE, "测试急停");
                std::this_thread::sleep_for(std::chrono::seconds(2));

                // 复位急停
                estop_manager->reset();
            }

            // 检查安全状态
            auto safety_status = safety_manager->getSafetyStatus();
            LOG_INFO("安全状态: 急停=%s, 警告=%d, 错误=%d",
                    safety_status.emergency_stop_active ? "激活" : "未激活",
                    safety_status.active_warnings,
                    safety_status.active_errors);

            // 显示统计信息
            auto stats = safety_manager->getStatistics();
            LOG_DEBUG("统计: 总事件=%d, 关键事件=%d, 平均响应时间=%.2fms",
                     stats.total_events, stats.critical_events, stats.avg_response_time * 1000);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 7. 演示急停保护模式
        LOG_INFO("=== 急停保护演示 ===");

        {
            // 使用RAII保护模式
            EStopGuard estop_guard(estop_manager, "危险操作保护");

            LOG_INFO("执行受保护的危险操作...");
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // 如果发生异常，Guard会自动激活急停
            // throw std::runtime_error("模拟异常");
        }

        LOG_INFO("保护操作完成，急停已自动复位");

        // 8. 演示安全检查装饰器
        LOG_INFO("=== 安全检查装饰器演示 ===");

        EStopStateChecker estop_checker(estop_manager);

        try {
            // 安全检查模式执行操作
            estop_checker.withSafetyCheck([]() {
                LOG_INFO("执行安全检查通过的操作");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                return true;
            });

            // 故意触发安全检查失败
            auto safety_passed = estop_manager->isSafetyCheckPassed();
            if (!safety_passed) {
                LOG_INFO("模拟安全检查失败...");
                // 下面这行会抛出异常
                // estop_checker.withSafetyCheck([]() {
                //     LOG_INFO("这行不会执行");
                //     return true;
                // });
            }

        } catch (const std::exception& e) {
            LOG_ERROR("安全检查失败: %s", e.what());
        }

        // 9. 停止监控
        LOG_INFO("停止所有监控...");
        safety_manager->stop();
        monitor.stop();
        estop_manager->stop();

        // 10. 显示历史记录
        auto events = safety_manager->getEventHistory(10);
        LOG_INFO("安全事件历史（最近%d条）:", events.size());
        for (const auto& event : events) {
            LOG_INFO("  - %s: %s", event.message.c_str(),
                     std::chrono::system_clock::to_time_t(event.timestamp));
        }

        // 11. 断开连接
        robot->disconnect();

    } catch (const std::exception& e) {
        LOG_ERROR("异常: %s", e.what());
        return 1;
    }

    LOG_INFO("安全监控演示完成");
    return 0;
}