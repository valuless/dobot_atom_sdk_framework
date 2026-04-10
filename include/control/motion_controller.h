/**
 * @file motion_controller.h
 * @brief 运动控制器
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_MOTION_CONTROLLER_H
#define DOBOT_ATOM_SDK_FRAMEWORK_MOTION_CONTROLLER_H

#include "motion_types.h"
#include "../utils/config_manager.h"
#include "../utils/logger.h"
#include <memory>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>

namespace robot_framework {

/**
 * @brief 运动控制器状态
 */
enum class ControllerState {
    IDLE,
    CONNECTING,
    READY,
    EXECUTING,
    PAUSED,
    STOPPING,
    ERROR
};

/**
 * @brief 运动控制器配置
 */
struct MotionControllerConfig {
    // 通信配置
    std::string robot_ip = "192.168.8.234";
    int robot_port = 50051;
    double communication_timeout = 5.0;  // s

    // 控制参数
    double control_frequency = 100.0;     // Hz
    double motion_timeout = 30.0;         // s
    bool enable_safety_checks = true;

    // 轨迹参数
    double max_joint_velocity = 2.0;       // rad/s
    double max_joint_acceleration = 4.0;   // rad/s^2
    double max_cartesian_velocity = 1.0;   // m/s
    double max_cartesian_acceleration = 2.0; // m/s^2

    // 安全参数
    double joint_limit_margin = 0.1;       // rad
    double workspace_limit_margin = 0.1;   // m
    bool enable_self_collision_check = true;
    double self_collision_margin = 0.05;    // m

    // 调试选项
    bool enable_debug_logging = false;
    bool enable_trajectory_logging = false;
    std::string log_file = "motion_controller.log";
};

/**
 * @brief 运动控制回调
 */
using MotionCallback = std::function<void(const MotionResult&)>;
using StateCallback = std::function<void(ControllerState)>;

/**
 * @brief 运动控制器类
 */
class MotionController {
public:
    MotionController();
    virtual ~MotionController();

    // 初始化和连接
    bool initialize(const MotionControllerConfig& config = MotionControllerConfig());
    bool connect();
    bool disconnect();
    bool isConnected() const;

    // 基本控制
    ControllerState getState() const;
    bool isReady() const;
    bool isExecuting() const;

    // 运动命令
    std::future<MotionResult> executeCommand(const MotionCommand& command);
    bool stopExecution();
    bool pauseExecution();
    bool resumeExecution();

    // 等待命令完成
    bool waitForCompletion(int command_id, double timeout = -1.0);
    MotionResult getResult(int command_id) const;

    // 回调管理
    void setMotionCallback(MotionCallback callback);
    void setStateCallback(StateCallback callback);
    void removeCallbacks();

    // 配置管理
    void setConfig(const MotionControllerConfig& config);
    MotionControllerConfig getConfig() const;
    bool reloadConfig();

    // 命令队列管理
    void setMaxQueueSize(size_t size);
    size_t getQueueSize() const;
    bool clearQueue();

    // 状态查询
    JointPoint getCurrentJointState() const;
    CartesianPoint getCurrentCartesianState() const;
    Vector6d getJointVelocities() const;

    // 安全检查
    bool checkJointLimits(const std::vector<double>& joints) const;
    bool checkWorkspaceLimit(const Vector3d& position) const;
    bool checkSelfCollision(const std::vector<double>& joints) const;

    // 轨迹规划
    std::shared_ptr<TrajectoryGenerator> getTrajectoryGenerator();
    void setTrajectoryGenerator(std::shared_ptr<TrajectoryGenerator> generator);

private:
    // 内部实现
    void communicationThread();
    void controlLoop();
    void processCommands();
    void sendCommandToRobot(const MotionCommand& command);
    void handleRobotResponse();
    void updateControllerState(ControllerState new_state);

    // 命令管理
    int generateCommandId();
    bool addToQueue(const MotionCommand& command);
    MotionCommand getNextCommand();
    void removeFromQueue(int command_id);

    // 安全检查
    bool performSafetyChecks(const MotionCommand& command);
    void emergencyStop();

    // 日志和调试
    void logCommand(const MotionCommand& command);
    void logResult(const MotionResult& result);
    void logError(const std::string& error);

private:
    // 配置
    MotionControllerConfig config_;
    mutable std::mutex config_mutex_;

    // 连接状态
    mutable std::mutex state_mutex_;
    ControllerState state_;
    std::string last_error_;

    // 通信
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> emergency_stop_{false};

    // 线程
    std::thread comm_thread_;
    std::thread control_thread_;
    std::mutex thread_mutex_;

    // 命令队列
    std::queue<MotionCommand> command_queue_;
    std::queue<MotionResult> result_queue_;
    std::unordered_map<int, MotionResult> results_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // 命令计数
    std::atomic<int> command_counter_{0};

    // 回调
    MotionCallback motion_callback_;
    StateCallback state_callback_;

    // 轨迹生成器
    std::shared_ptr<TrajectoryGenerator> trajectory_generator_;

    // 当前状态
    JointPoint current_joint_state_;
    CartesianPoint current_cartesian_state_;
    std::mutex state_data_mutex_;
};

/**
 * @brief 简单轨迹生成器实现
 */
class SimpleTrajectoryGenerator : public TrajectoryGenerator {
public:
    bool generateTrajectory(
        const JointPoint& start,
        const JointPoint& goal,
        Trajectory& trajectory,
        const MotionPlanningParams& params) override;

    bool generateTrajectory(
        const CartesianPoint& start,
        const CartesianPoint& goal,
        Trajectory& trajectory,
        const MotionPlanningParams& params) override;

    bool generateTrajectory(
        const std::vector<JointPoint>& waypoints,
        Trajectory& trajectory,
        const MotionPlanningParams& params) override;

private:
    // 内部方法
    void generateLinearTrajectory(
        const JointPoint& start,
        const JointPoint& goal,
        Trajectory& trajectory,
        const MotionPlanningParams& params);

    void generateSplineTrajectory(
        const std::vector<JointPoint>& waypoints,
        Trajectory& trajectory,
        const MotionPlanningParams& params);
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_MOTION_CONTROLLER_H