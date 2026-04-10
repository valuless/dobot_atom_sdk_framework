/**
 * @file dobot_robot.cpp
 * @brief Dobot Atom 机器人实现类
 * @author Framework
 * @date 2024
 */

#include "robot/dobot_robot.h"
#include "control/motion_controller.h"
#include "monitor/state_monitor.h"
#include "safety/safety_manager.h"
#include "utils/logger.h"
#include "utils/config_manager.h"
#include <thread>
#include <chrono>

namespace robot_framework {

DobotRobot::DobotRobot()
    : state_(RobotState::DISCONNECTED)
    , motion_controller_(std::make_unique<MotionController>())
    , state_monitor_(std::make_unique<StateMonitor>(this))
    , safety_manager_(std::make_unique<SafetyManager>(state_monitor_.get()))
    , current_mode_(0)
    , rpc_handle_(nullptr)
    , dds_handle_(nullptr) {

    LOG_INFO("DobotRobot 实例已创建");
}

DobotRobot::~DobotRobot() {
    disconnect();
    LOG_INFO("DobotRobot 实例已销毁");
}

bool DobotRobot::connect(const std::string& ip, int port) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ == RobotState::CONNECTED) {
        LOG_WARN("机器人已经连接");
        return true;
    }

    updateState(RobotState::CONNECTING);

    try {
        ip_ = ip;
        port_ = port;

        // 初始化RPC连接
        LOG_INFO("正在连接到机器人 %s:%d", ip.c_str(), port);
        if (!initializeConnection()) {
            updateState(RobotState::ERROR);
            return false;
        }

        // 初始化DDS连接
        // 这里需要添加DDS初始化代码
        LOG_INFO("DDS连接初始化...");

        // 启动监控线程
        running_ = true;
        bg_thread_ = std::thread(&DobotRobot::backgroundThread, this);
        monitor_thread_ = std::thread(&DobotRobot::monitorThread, this);

        updateState(RobotState::CONNECTED);
        LOG_INFO("机器人连接成功");

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("连接失败: %s", e.what());
        updateState(RobotState::ERROR);
        return false;
    }
}

bool DobotRobot::disconnect() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ == RobotState::DISCONNECTED) {
        return true;
    }

    // 停止所有线程
    running_ = false;
    should_stop_ = true;

    if (bg_thread_.joinable()) {
        bg_thread_.join();
    }

    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }

    // 清理连接
    cleanupConnection();

    updateState(RobotState::DISCONNECTED);
    LOG_INFO("机器人已断开连接");

    return true;
}

bool DobotRobot::isConnected() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_ == RobotState::CONNECTED;
}

RobotState DobotRobot::getState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_;
}

void DobotRobot::setStateCallback(StateCallback callback) {
    state_callback_ = callback;
}

void DobotRobot::setErrorCallback(ErrorCallback callback) {
    error_callback_ = callback;
}

bool DobotRobot::emergencyStop() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (emergency_stop_) {
        LOG_WARN("急停已经激活");
        return true;
    }

    LOG_WARN("激活急停...");
    emergency_stop_ = true;

    // 发送急停命令到机器人
    // 这里需要添加RPC急停命令
    // bool result = rpc_send_emergency_stop(rpc_handle_);

    updateState(RobotState::EMERGENCY_STOP);

    // 通知安全系统
    if (safety_manager_) {
        safety_manager_->emergencyStop("软件急停");
    }

    return true;
}

bool DobotRobot::resetEmergencyStop() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (!emergency_stop_) {
        LOG_WARN("急停未激活");
        return true;
    }

    LOG_INFO("复位急停...");
    emergency_stop_ = false;

    // 发送复位命令到机器人
    // 这里需要添加RPC复位命令
    // bool result = rpc_send_reset_emergency_stop(rpc_handle_);

    updateState(RobotState::READY);

    return true;
}

bool DobotRobot::isEmergencyStop() const {
    return emergency_stop_;
}

std::string DobotRobot::getRobotModel() const {
    // 这里需要从机器人获取实际型号
    return "Dobot Atom";
}

std::string DobotRobot::getFirmwareVersion() const {
    // 这里需要从机器人获取实际版本
    return "1.0.0";
}

bool DobotRobot::isSafeToOperate() const {
    if (!safety_manager_) {
        return true;
    }

    auto status = safety_manager_->getSafetyStatus();
    return !status.emergency_stop_active && status.active_warnings == 0;
}

bool DobotRobot::getLastError(std::string& error) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    error = last_error_;
    return !last_error_.empty();
}

bool DobotRobot::setMode(int mode) {
    if (!isConnected()) {
        LOG_ERROR("未连接到机器人");
        return false;
    }

    // 这里需要添加RPC模式切换代码
    // bool result = rpc_send_mode_switch(rpc_handle_, mode);

    current_mode_ = mode;
    return true;
}

int DobotRobot::getCurrentMode() const {
    return current_mode_;
}

bool DobotRobot::executeMotion(const MotionCommand& command) {
    if (!isConnected()) {
        LOG_ERROR("未连接到机器人");
        return false;
    }

    if (emergency_stop_) {
        LOG_ERROR("急停状态，无法执行运动");
        return false;
    }

    // 使用运动控制器执行命令
    auto future = motion_controller_->executeCommand(command);
    return true;
}

bool DobotRobot::executeTrajectory(const Trajectory& trajectory) {
    MotionCommand command;
    command.type = MotionCommand::Type::EXECUTE_TRAJECTORY;
    command.trajectory = trajectory;
    command.wait_for_completion = true;

    return executeMotion(command);
}

void DobotRobot::updateState(RobotState new_state) {
    setStateInternal(new_state);

    // 通知回调
    if (state_callback_) {
        state_callback_(new_state);
    }
}

void DobotRobot::setStateInternal(RobotState state) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ != state) {
        LOG_DEBUG("状态变更: %d -> %d", (int)state_, (int)state);
        state_ = state;

        // 通知状态变量等待
        state_cv_.notify_all();
    }
}

void DobotRobot::processStateUpdate() {
    // 这里需要实现实际的状态更新逻辑
    // 从机器人获取状态数据
}

bool DobotRobot::initializeConnection() {
    // 这里需要实现实际的RPC初始化代码
    LOG_INFO("初始化RPC连接...");

    // 模拟连接初始化
    // rpc_handle_ = rpc_connect(ip_.c_str(), port_);
    // if (!rpc_handle_) {
    //     LOG_ERROR("RPC连接失败");
    //     return false;
    // }

    // 初始化运动控制器
    MotionControllerConfig config;
    config.robot_ip = ip_;
    config.robot_port = port_;

    if (!motion_controller_->initialize(config)) {
        LOG_ERROR("运动控制器初始化失败");
        return false;
    }

    return true;
}

void DobotRobot::cleanupConnection() {
    LOG_INFO("清理连接...");

    // 清理RPC连接
    if (rpc_handle_) {
        // rpc_disconnect(rpc_handle_);
        rpc_handle_ = nullptr;
    }

    // 清理DDS连接
    if (dds_handle_) {
        // dds_cleanup(dds_handle_);
        dds_handle_ = nullptr;
    }
}

void DobotRobot::backgroundThread() {
    LOG_INFO("后台线程启动");

    while (running_) {
        if (!should_stop_) {
            // 处理状态更新
            processStateUpdate();

            // 检查急停状态
            if (emergency_stop_) {
                // 处理急停逻辑
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_INFO("后台线程停止");
}

void DobotRobot::monitorThread() {
    LOG_INFO("监控线程启动");

    while (running_) {
        if (!should_stop_) {
            // 启动状态监控
            if (!state_monitor_->isRunning()) {
                state_monitor_->start();
            }

            // 启动安全管理
            if (!safety_manager_->isRunning()) {
                safety_manager_->start();
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 停止监控
    if (state_monitor_->isRunning()) {
        state_monitor_->stop();
    }

    if (safety_manager_->isRunning()) {
        safety_manager_->stop();
    }

    LOG_INFO("监控线程停止");
}

// 工厂方法实现
std::unique_ptr<RobotInterface> RobotFactory::createRobot() {
    return std::make_unique<DobotRobot>();
}

std::unique_ptr<RobotInterface> RobotFactory::createRobot(const std::string& type) {
    if (type == "dobot" || type.empty()) {
        return createRobot();
    }

    LOG_ERROR("不支持的机器人类型: %s", type.c_str());
    return nullptr;
}

} // namespace robot_framework