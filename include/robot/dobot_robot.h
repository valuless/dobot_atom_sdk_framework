/**
 * @file dobot_robot.h
 * @brief Dobot Atom 机器人实现类
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_DOBOT_ROBOT_H
#define DOBOT_ATOM_SDK_FRAMEWORK_DOBOT_ROBOT_H

#include "robot_interface.h"
#include "../control/motion_controller.h"
#include "../monitor/state_monitor.h"
#include "../safety/safety_manager.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace robot_framework {

/**
 * @brief Dobot Atom 机器人实现类
 */
class DobotRobot : public RobotInterface {
public:
    DobotRobot();
    virtual ~DobotRobot();

    // 基本连接管理
    bool connect(const std::string& ip, int port = 50051) override;
    bool disconnect() override;
    bool isConnected() const override;
    RobotState getState() const override;

    // 状态回调设置
    void setStateCallback(StateCallback callback) override;
    void setErrorCallback(ErrorCallback callback) override;

    // 安全控制
    bool emergencyStop() override;
    bool resetEmergencyStop() override;
    bool isEmergencyStop() const override;

    // 系统信息
    std::string getRobotModel() const override;
    std::string getFirmwareVersion() const override;

    // 状态查询接口
    bool isSafeToOperate() const override;
    bool getLastError(std::string& error) const override;

    // 运动控制接口
    bool setMode(int mode);
    int getCurrentMode() const;

    // 高级控制接口
    bool executeMotion(const MotionCommand& command);
    bool executeTrajectory(const Trajectory& trajectory);

private:
    // 内部状态管理
    void updateState(RobotState new_state);
    void setStateInternal(RobotState state);
    void processStateUpdate();

    // 连接管理
    bool initializeConnection();
    void cleanupConnection();

    // 线程管理
    void backgroundThread();
    void monitorThread();

private:
    std::string ip_;
    int port_;

    // 连接状态
    mutable std::mutex state_mutex_;
    std::condition_variable state_cv_;
    RobotState state_;
    std::string last_error_;

    // 回调函数
    StateCallback state_callback_;
    ErrorCallback error_callback_;

    // 核心组件
    std::unique_ptr<MotionController> motion_controller_;
    std::unique_ptr<StateMonitor> state_monitor_;
    std::unique_ptr<SafetyManager> safety_manager_;

    // 线程
    std::thread bg_thread_;
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> emergency_stop_{false};

    // 模式信息
    int current_mode_;

    // 连接句柄（示例）
    void* rpc_handle_;
    void* dds_handle_;
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_DOBOT_ROBOT_H