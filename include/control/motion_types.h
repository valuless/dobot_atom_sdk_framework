/**
 * @file motion_types.h
 * @brief 运动控制相关数据类型定义
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_MOTION_TYPES_H
#define DOBOT_ATOM_SDK_FRAMEWORK_MOTION_TYPES_H

#include <vector>
#include <string>
#include <chrono>
#include <Eigen/Dense>

namespace robot_framework {

/**
 * @brief 使用Eigen库定义常用向量类型
 */
using Vector3d = Eigen::Vector3d;
using Vector6d = Eigen::Matrix<double, 6, 1>;
using Vector7d = Eigen::Matrix<double, 7, 1>;
using Matrix3d = Eigen::Matrix3d;
using Matrix4d = Eigen::Matrix4d;

/**
 * @brief 关节空间点
 */
struct JointPoint {
    std::vector<double> positions;    // 关节角度 (rad)
    std::vector<double> velocities;   // 关节速度 (rad/s)
    std::vector<double> accelerations; // 关节加速度 (rad/s^2)
    std::vector<double> efforts;      // 关节力矩 (Nm)

    // 时间信息
    double duration;                  // 持续时间 (s)
    double time_from_start;          // 从开始时间 (s)

    // 元数据
    std::string description;
    int waypoint_id;
};

/**
 * @brief 笛卡尔空间点
 */
struct CartesianPoint {
    Vector3d position;               // 位置 (m)
    Vector3d orientation;            // 欧拉角 (rad)
    Vector3d linear_velocity;        // 线速度 (m/s)
    Vector3d angular_velocity;       // 角速度 (rad/s)

    // 时间信息
    double duration;
    double time_from_start;

    // 元数据
    std::string description;
    int waypoint_id;
};

/**
 * @brief 力控参数
 */
struct ForceControlParams {
    Vector3d force_threshold;       // 力阈值 (N)
    Vector3d torque_threshold;       // 力矩阈值 (Nm)
    double damping;                  // 阻尼系数
    double stiffness;                // 刚度系数
    bool enable_collision_detection; // 启用碰撞检测
};

/**
 * @brief 运动控制模式
 */
enum class MotionMode {
    JOINT,          // 关节空间运动
    CARTESIAN,      // 笛卡尔空间运动
    VELOCITY,       // 速度控制
    FORCE,          // 力控
    HYBRID,         // 混合控制
    IDLE           // 空闲
};

/**
 * @brief 运动规划参数
 */
struct MotionPlanningParams {
    MotionMode mode = MotionMode::JOINT;

    // 轨迹参数
    double max_velocity = 1.0;       // 最大速度
    double max_acceleration = 2.0;   // 最大加速度
    double max_jerk = 5.0;          // 最大跃度

    // 平滑参数
    bool enable_smoothing = true;    // 启用轨迹平滑
    double smoothing_time = 0.1;     // 平滑时间

    // 安全参数
    bool check_self_collision = true; // 自碰撞检测
    bool check_workspace_limit = true; // 工作空间限制
    double safety_margin = 0.05;      // 安全边界 (m)

    // 优化参数
    double planning_time_limit = 5.0; // 规划时间限制
    bool enable_replanning = true;   // 启用重新规划
};

/**
 * @brief 轨迹点
 */
struct TrajectoryPoint {
    double timestamp;                // 时间戳
    Vector6d joints;                // 关节角度
    Vector6d joint_velocities;      // 关节速度
    Vector6d joint_accelerations;   // 关节加速度

    // 可选笛卡尔信息
    Vector3d position;              // 位置
    Vector3d orientation;           // 方向
    Vector3d linear_vel;            // 线速度
    Vector3d angular_vel;           // 角速度

    // 元数据
    int waypoint_id;
    std::string description;
};

/**
 * @brief 轨迹
 */
struct Trajectory {
    std::vector<TrajectoryPoint> points;
    MotionPlanningParams params;

    // 轨迹信息
    std::string name;
    std::string description;
    double total_duration;
    int point_count;

    // 状态
    enum class State {
        IDLE,
        PLANNING,
        EXECUTING,
        PAUSED,
        COMPLETED,
        FAILED
    } state = State::IDLE;

    // 方法
    void clear();
    bool empty() const;
    double getDuration() const;
    void addPoint(const TrajectoryPoint& point);
    TrajectoryPoint interpolate(double time) const;
};

/**
 * @brief 运动命令
 */
struct MotionCommand {
    enum class Type {
        MOVE_JOINTS,        // 移动到关节位置
        MOVE_CARTESIAN,    // 移动到笛卡尔位置
        VELOCITY_CMD,       // 速度命令
        EXECUTE_TRAJECTORY // 执行轨迹
    };

    Type type;
    int robot_id;  // 目标机器人

    // 命令参数
    union {
        JointPoint joint_point;
        CartesianPoint cartesian_point;
        struct {
            Vector3d linear_vel;
            Vector3d angular_vel;
            double duration;
        } velocity_cmd;
        Trajectory trajectory;
    };

    // 执行选项
    bool async = false;              // 异步执行
    bool wait_for_completion = true;  // 等待完成
    double timeout = 10.0;            // 超时时间

    // 元数据
    std::string command_id;
    std::string description;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief 运动结果
 */
struct MotionResult {
    enum class Status {
        SUCCESS,
        FAILURE,
        TIMEOUT,
        ABORTED,
        ERROR
    };

    Status status;
    std::string message;
    double execution_time;
    double planned_time;
    int executed_points;
    std::vector<std::string> warnings;

    // 错误信息
    struct ErrorInfo {
        int error_code;
        std::string error_message;
        std::string component;
    };

    ErrorInfo error;
};

/**
 * @brief 轨迹生成器接口
 */
class TrajectoryGenerator {
public:
    virtual ~TrajectoryGenerator() = default;

    virtual bool generateTrajectory(
        const JointPoint& start,
        const JointPoint& goal,
        Trajectory& trajectory,
        const MotionPlanningParams& params = MotionPlanningParams()) = 0;

    virtual bool generateTrajectory(
        const CartesianPoint& start,
        const CartesianPoint& goal,
        Trajectory& trajectory,
        const MotionPlanningParams& params = MotionPlanningParams()) = 0;

    virtual bool generateTrajectory(
        const std::vector<JointPoint>& waypoints,
        Trajectory& trajectory,
        const MotionPlanningParams& params = MotionPlanningParams()) = 0;
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_MOTION_TYPES_H