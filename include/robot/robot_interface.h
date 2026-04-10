/**
 * @file robot_interface.h
 * @brief Dobot Atom 机器人接口基类
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_ROBOT_INTERFACE_H
#define DOBOT_ATOM_SDK_FRAMEWORK_ROBOT_INTERFACE_H

#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <thread>

namespace robot_framework {

/**
 * @brief 机器人接口状态枚举
 */
enum class RobotState {
    DISCONNECTED,    // 未连接
    CONNECTING,      // 连接中
    CONNECTED,      // 已连接
    ERROR,          // 错误状态
    EMERGENCY_STOP,  // 急停
    READY,          // 准备就绪
    RUNNING         // 运行中
};

/**
 * @brief 机器人接口抽象基类
 */
class RobotInterface {
public:
    using StateCallback = std::function<void(RobotState)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    virtual ~RobotInterface() = default;

    // 基本连接管理
    virtual bool connect(const std::string& ip, int port = 50051) = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual RobotState getState() const = 0;

    // 状态回调设置
    virtual void setStateCallback(StateCallback callback) = 0;
    virtual void setErrorCallback(ErrorCallback callback) = 0;

    // 安全控制
    virtual bool emergencyStop() = 0;
    virtual bool resetEmergencyStop() = 0;
    virtual bool isEmergencyStop() const = 0;

    // 系统信息
    virtual std::string getRobotModel() const = 0;
    virtual std::string getFirmwareVersion() const = 0;

    // 状态查询接口
    virtual bool isSafeToOperate() const = 0;
    virtual bool getLastError(std::string& error) const = 0;
};

/**
 * @brief 机器人接口工厂类
 */
class RobotFactory {
public:
    static std::unique_ptr<RobotInterface> createRobot();
    static std::unique_ptr<RobotInterface> createRobot(const std::string& type);
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_ROBOT_INTERFACE_H