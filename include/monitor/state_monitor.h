/**
 * @file state_monitor.h
 * @brief 状态监控器
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_STATE_MONITOR_H
#define DOBOT_ATOM_SDK_FRAMEWORK_STATE_MONITOR_H

#include "robot_interface.h"
#include "../utils/config_manager.h"
#include "../utils/logger.h"
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <chrono>

namespace robot_framework {

/**
 * @brief 状态监控数据结构
 */
struct RobotStateData {
    // 时间戳
    std::chrono::system_clock::time_point timestamp;

    // FSM状态
    int fsm_id;
    std::string fsm_name;

    // 关节状态
    struct JointState {
        double position;
        double velocity;
        double effort;
        int32_t error_code;
        uint8_t servo_state;
        uint8_t temperature;
    };

    std::unordered_map<std::string, JointState> joints;

    // 系统状态
    struct SystemState {
        uint8_t overall_status;
        uint16_t error_code;
        uint16_t warning_code;
        bool emergency_stop;
        double battery_voltage;
        double cpu_usage;
        double memory_usage;
    } system;

    // 传感器数据
    struct SensorData {
        struct ImuData {
            double accel_x, accel_y, accel_z;
            double gyro_x, gyro_y, gyro_z;
        } imu;

        struct FootForce {
            double left_front;
            double left_rear;
            double right_front;
            double right_rear;
        } foot_force;
    } sensors;
};

/**
 * @brief 状态监控配置
 */
struct MonitorConfig {
    double update_frequency = 100.0;  // Hz
    bool enable_joint_monitoring = true;
    bool enable_system_monitoring = true;
    bool enable_sensor_monitoring = true;
    double joint_temperature_threshold = 60.0;  // °C
    double error_code_threshold = 1;
    bool enable_logging = true;
    std::string log_file = "robot_state.log";
};

/**
 * @brief 状态监控回调函数类型
 */
using StateUpdateCallback = std::function<void(const RobotStateData&)>;
using ErrorCallback = std::function<void(const std::string&, int)>;

/**
 * @brief 状态监控器类
 */
class StateMonitor {
public:
    StateMonitor(RobotInterface* robot);
    virtual ~StateMonitor();

    // 基本控制
    bool start();
    bool stop();
    bool isRunning() const;
    void pause();
    void resume();

    // 配置管理
    void setConfig(const MonitorConfig& config);
    MonitorConfig getConfig() const;

    // 回调管理
    void setStateCallback(StateUpdateCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void removeCallbacks();

    // 数据获取
    RobotStateData getCurrentState() const;
    std::vector<RobotStateData> getStateHistory(int count = 100) const;
    std::vector<RobotStateData> getStateHistory(const std::chrono::system_clock::time_point& start,
                                               const std::chrono::system_clock::time_point& end) const;

    // 实时监控
    void monitorJointTemperatures();
    void monitorErrorCodes();
    void monitorSystemHealth();

    // 统计信息
    struct Statistics {
        double avg_joint_temperature;
        double max_joint_temperature;
        int total_error_count;
        int warning_count;
        double uptime;
    };

    Statistics getStatistics() const;

private:
    // 监控线程
    void monitoringThread();
    void processStateUpdates();
    void checkConditions();

    // 数据处理
    void updateRobotState();
    void storeStateData(const RobotStateData& state);
    void logStateData(const RobotStateData& state);

    // 状态检查
    bool checkJointTemperatures(const RobotStateData& state);
    bool checkErrorCodes(const RobotStateData& state);
    bool checkSystemHealth(const RobotStateData& state);

private:
    RobotInterface* robot_;

    // 配置
    MonitorConfig config_;
    mutable std::mutex config_mutex_;

    // 运行状态
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> should_stop_{false};

    // 线程
    std::thread monitor_thread_;
    std::thread process_thread_;

    // 数据存储
    mutable std::mutex data_mutex_;
    std::queue<RobotStateData> state_queue_;
    std::vector<RobotStateData> state_history_;

    // 回调
    StateUpdateCallback state_callback_;
    ErrorCallback error_callback_;

    // 统计信息
    mutable std::mutex stats_mutex_;
    Statistics statistics_;
    std::chrono::system_clock::time_point start_time_;

    // 配置文件路径
    std::string config_file_path_;
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_STATE_MONITOR_H