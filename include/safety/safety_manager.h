/**
 * @file safety_manager.h
 * @brief 安全管理器
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_SAFETY_MANAGER_H
#define DOBOT_ATOM_SDK_FRAMEWORK_SAFETY_MANAGER_H

#include "../control/motion_types.h"
#include "../monitor/state_monitor.h"
#include "../utils/config_manager.h"
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace robot_framework {

/**
 * @brief 安全事件类型
 */
enum class SafetyEventType {
    // 硬件事件
    HARDWARE_ERROR,
    OVERHEATING,
    MOTOR_CURRENT_EXCEEDED,
    ENCODER_ERROR,
    COMMUNICATION_FAILURE,

    // 软件事件
    SOFTWARE_ERROR,
    MEMORY_EXCEEDED,
    CPU_OVERLOAD,

    // 操作事件
    EMERGENCY_STOP,
    COLLISION_DETECTED,
    WORKSPACE_VIOLATION,
    JOINT_LIMIT_VIOLATION,

    // 系统事件
    BATTERY_LOW,
    POWER_LOSS,
    SAFE_MODE_ACTIVATED
};

/**
 * @brief 安全事件严重级别
 */
enum class SafetySeverity {
    INFO,      // 信息
    WARNING,   // 警告
    ERROR,     // 错误
    CRITICAL,  // 严重
    FATAL      // 致命
};

/**
 * @brief 安全事件结构
 */
struct SafetyEvent {
    SafetyEventType type;
    SafetySeverity severity;
    std::string message;
    std::string component;
    int error_code;

    // 时间信息
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;

    // 上下文信息
    std::unordered_map<std::string, double> values;
    std::unordered_map<std::string, std::string> metadata;

    // 状态
    enum class State {
        ACTIVE,
        ACKNOWLEDGED,
        RESOLVED,
        SUPPRESSED
    } state = State::ACTIVE;
};

/**
 * @brief 安全规则接口
 */
class SafetyRule {
public:
    virtual ~SafetyRule() = default;

    // 规则评估
    virtual bool evaluate(const RobotStateData& state, SafetyEvent& event) = 0;

    // 规则名称和描述
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;

    // 规则配置
    virtual void configure(const ConfigManager::Json& config) = 0;
    virtual ConfigManager::Json getConfig() const = 0;

    // 规则状态
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
};

/**
 * @brief 温度监控规则
 */
class TemperatureRule : public SafetyRule {
public:
    TemperatureRule();
    bool evaluate(const RobotStateData& state, SafetyEvent& event) override;
    std::string getName() const override { return "Temperature Monitor"; }
    std::string getDescription() const override { return "Monitor joint motor temperatures"; }
    void configure(const ConfigManager::Json& config) override;
    ConfigManager::Json getConfig() const override;
    bool isEnabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }

private:
    bool enabled_ = true;
    double warning_threshold_ = 60.0;
    double critical_threshold_ = 75.0;
    std::unordered_map<std::string, double> max_temperatures_;
};

/**
 * @brief 碰撞检测规则
 */
class CollisionRule : public SafetyRule {
public:
    CollisionRule();
    bool evaluate(const RobotStateData& state, SafetyEvent& event) override;
    std::string getName() const override { return "Collision Detection"; }
    std::string getDescription() const override { return "Detect self-collisions and collisions with environment"; }
    void configure(const ConfigManager::Json& config) override;
    ConfigManager::Json getConfig() const override;
    bool isEnabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }

private:
    bool enabled_ = true;
    double collision_threshold_ = 0.05;  // m
    bool check_self_collision_ = true;
    bool check_environment_collision_ = false;
};

/**
 * @brief 工作空间限制规则
 */
class WorkspaceRule : public SafetyRule {
public:
    WorkspaceRule();
    bool evaluate(const RobotStateData& state, SafetyEvent& event) override;
    std::string getName() const override { return "Workspace Limit"; }
    std::string getDescription() const override { return "Ensure robot stays within operational workspace"; }
    void configure(const ConfigManager::Json& config) override;
    ConfigManager::Json getConfig() const override;
    bool isEnabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }

private:
    bool enabled_ = true;
    struct Bounds {
        double min_x, max_x;
        double min_y, max_y;
        double min_z, max_z;
    } bounds_;
    double margin_ = 0.1;
};

/**
 * @brief 安全管理器配置
 */
struct SafetyManagerConfig {
    // 监控配置
    double monitoring_frequency = 100.0;  // Hz
    bool enable_all_rules = true;
    double emergency_stop_timeout = 1.0;  // s

    // 事件配置
    size_t max_event_history = 1000;
    bool enable_auto_recovery = false;
    std::string recovery_script = "";

    // 日志配置
    bool enable_event_logging = true;
    std::string log_file = "safety_manager.log";
    std::string event_database = "safety_events.db";

    // 通知配置
    bool enable_email_notifications = false;
    std::string email_recipients = "";
    bool enable_sms_notifications = false;
    std::string sms_recipients = "";
};

/**
 * @brief 安全事件回调
 */
using SafetyEventCallback = std::function<void(const SafetyEvent&)>;
using SafetyActionCallback = std::function<void(const SafetyEvent&)>;

/**
 * @brief 安全管理器类
 */
class SafetyManager {
public:
    SafetyManager(StateMonitor* monitor);
    ~SafetyManager();

    // 初始化
    bool initialize(const SafetyManagerConfig& config = SafetyManagerConfig());
    bool start();
    bool stop();
    bool isRunning() const;

    // 规则管理
    void addRule(std::shared_ptr<SafetyRule> rule);
    void removeRule(const std::string& name);
    void enableRule(const std::string& name, bool enable);
    std::vector<std::string> getAvailableRules() const;
    std::vector<std::string> getEnabledRules() const;

    // 事件管理
    void handleEvent(const SafetyEvent& event);
    std::vector<SafetyEvent> getEventHistory(int count = 100) const;
    std::vector<SafetyEvent> getEventsByType(SafetyEventType type) const;
    std::vector<SafetyEvent> getEventsBySeverity(SafetySeverity severity) const;
    bool acknowledgeEvent(int event_id);
    bool resolveEvent(int event_id, const std::string& resolution = "");

    // 回调管理
    void setEventCallback(SafetyEventCallback callback);
    void setActionCallback(SafetyActionCallback callback);

    // 安全控制
    bool emergencyStop(const std::string& reason = "");
    bool resetEmergencyStop();
    bool isEmergencyStop() const;
    bool isSafeToOperate() const;

    // 状态查询
    struct SafetyStatus {
        bool emergency_stop_active;
        int active_warnings;
        int active_errors;
        std::vector<std::string> violated_rules;
        double safety_score;
        std::chrono::system_clock::time_point last_violation;
    };

    SafetyStatus getSafetyStatus() const;

    // 统计信息
    struct SafetyStatistics {
        int total_events;
        int critical_events;
        int warnings;
        double avg_response_time;
        double uptime_percentage;
    };

    SafetyStatistics getStatistics() const;

private:
    // 监控线程
    void monitoringThread();
    void evaluateRules(const RobotStateData& state);

    // 事件处理
    void processEvent(const SafetyEvent& event);
    void logEvent(const SafetyEvent& event);
    void notifyEvent(const SafetyEvent& event);
    void executeAction(const SafetyEvent& event);

    // 规则评估
    void loadDefaultRules();
    void configureRules();

    // 事件存储
    void storeEvent(const SafetyEvent& event);
    SafetyEvent generateEventId(const SafetyEvent& event);

private:
    StateMonitor* monitor_;

    // 配置
    SafetyManagerConfig config_;
    mutable std::mutex config_mutex_;

    // 运行状态
    std::atomic<bool> running_{false};
    std::atomic<bool> emergency_stop_{false};

    // 规则管理
    std::unordered_map<std::string, std::shared_ptr<SafetyRule>> rules_;
    mutable std::mutex rules_mutex_;

    // 事件管理
    std::vector<SafetyEvent> event_history_;
    mutable std::mutex event_mutex_;
    std::atomic<int> event_counter_{0};

    // 回调
    SafetyEventCallback event_callback_;
    SafetyActionCallback action_callback_;

    // 线程
    std::thread monitor_thread_;
    std::mutex thread_mutex_;

    // 状态跟踪
    std::chrono::system_clock::time_point last_violation_;
    SafetyStatistics statistics_;
    mutable std::mutex stats_mutex_;
};

/**
 * @brief 安全管理器工厂
 */
class SafetyManagerFactory {
public:
    static std::unique_ptr<SafetyManager> createSafetyManager(StateMonitor* monitor);
    static std::unique_ptr<SafetyManager> createBasicSafetyManager(StateMonitor* monitor);
    static std::unique_ptr<SafetyManager> createComprehensiveSafetyManager(StateMonitor* monitor);
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_SAFETY_MANAGER_H