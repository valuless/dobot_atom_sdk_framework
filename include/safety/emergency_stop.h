/**
 * @file emergency_stop.h
 * @brief 急停管理器
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_EMERGENCY_STOP_H
#define DOBOT_ATOM_SDK_FRAMEWORK_EMERGENCY_STOP_H

#include "../utils/logger.h"
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace robot_framework {

/**
 * @brief 急停源类型
 */
enum class EStopSource {
    HARDWARE,     // 硬件急停按钮
    SOFTWARE,     // 软件急停
    REMOTE,       // 远程急停
    SAFETY_SYSTEM // 安全系统触发
};

/**
 * @brief 急停状态
 */
enum class EStopState {
    INACTIVE,     // 未激活
    ACTIVE,       // 已激活
    ACKNOWLEDGED, // 已确认
    RESET         // 已复位
};

/**
 * @brief 急停事件
 */
struct EStopEvent {
    EStopState old_state;
    EStopState new_state;
    EStopSource source;
    std::string reason;
    std::chrono::system_clock::time_point timestamp;
    std::string operator_id;
};

/**
 * @brief 急停配置
 */
struct EStopConfig {
    // 自动复位配置
    bool auto_reset_after = false;
    double auto_reset_delay = 5.0;  // seconds
    std::string auto_reset_conditions = "";

    // 安全配置
    double safety_check_interval = 0.1;  // seconds
    bool require_acknowledgment = true;
    bool enable_hardware_monitoring = true;

    // 通知配置
    bool enable_notifications = true;
    std::string notification_method = "email";
    std::string notification_recipients = "";

    // 日志配置
    bool enable_detailed_logging = true;
    std::string log_file = "emergency_stop.log";
};

/**
 * @brief 急停管理器类
 */
class EmergencyStopManager {
public:
    using StateCallback = std::function<void(const EStopEvent&)>;
    using SafetyCheckCallback = std::function<bool()>;

    EmergencyStopManager();
    virtual ~EmergencyStopManager();

    // 初始化
    bool initialize(const EStopConfig& config = EStopConfig());
    bool start();
    bool stop();
    bool isRunning() const;

    // 急停控制
    bool activate(EStopSource source, const std::string& reason = "");
    bool acknowledge(const std::string& operator_id = "");
    bool reset();
    bool isActivated() const;
    EStopState getState() const;

    // 硬件监控
    bool enableHardwareMonitoring(bool enable);
    bool addHardwareButton(int button_id);
    bool removeHardwareButton(int button_id);

    // 安全检查
    void setSafetyCheck(SafetyCheckCallback callback);
    bool performSafetyCheck();
    bool isSafetyCheckPassed() const;

    // 回调管理
    void setStateCallback(StateCallback callback);
    void removeCallbacks();

    // 状态查询
    struct StatusInfo {
        EStopState state;
        EStopSource source;
        std::string reason;
        std::chrono::system_clock::time_point activation_time;
        std::chrono::system_clock::time_point acknowledge_time;
        bool safety_check_passed;
        int active_sources;
        std::string last_operator;
    };

    StatusInfo getStatus() const;

    // 历史记录
    std::vector<EStopEvent> getEventHistory(int count = 100) const;
    std::vector<EStopEvent> getEventsBySource(EStopSource source) const;

    // 配置管理
    void setConfig(const EStopConfig& config);
    EStopConfig getConfig() const;

private:
    // 内部方法
    void monitoringThread();
    void handleButtonPress(int button_id);
    void checkAutoReset();
    void notifyStateChange(const EStopEvent& event);
    void logEvent(const EStopEvent& event);

    // 状态管理
    void setState(EStopState new_state, EStopSource source, const std::string& reason = "");
    void updateActivationTime();
    void updateAcknowledgeTime();

private:
    // 配置
    EStopConfig config_;
    mutable std::mutex config_mutex_;

    // 运行状态
    std::atomic<bool> running_{false};
    std::atomic<bool> hardware_monitoring_{false};

    // 急停状态
    EStopState state_ = EStopState::INACTIVE;
    EStopSource activation_source_ = EStopSource::SOFTWARE;
    std::string activation_reason_;
    std::string acknowledge_operator_;
    std::chrono::system_clock::time_point activation_time_;
    std::chrono::system_clock::time_point acknowledge_time_;

    // 安全检查
    SafetyCheckCallback safety_check_callback_;
    bool safety_check_passed_ = true;

    // 硬件按钮
    std::vector<int> hardware_buttons_;
    std::mutex buttons_mutex_;

    // 事件历史
    std::vector<EStopEvent> event_history_;
    mutable std::mutex history_mutex_;

    // 回调
    StateCallback state_callback_;

    // 线程
    std::thread monitor_thread_;
    std::mutex thread_mutex_;
    std::atomic<bool> should_stop_{false};
};

/**
 * @brief 急停状态检查器（装饰器模式）
 */
class EStopStateChecker {
public:
    EStopStateChecker(std::shared_ptr<EmergencyStopManager> estop_manager)
        : estop_manager_(estop_manager) {}

    // 操作前的安全检查
    template<typename Func>
    auto withSafetyCheck(Func&& func) -> decltype(func()) {
        if (!estop_manager_ || estop_manager_->isActivated()) {
            throw std::runtime_error("Emergency stop is active - operation blocked");
        }

        if (!estop_manager_->isSafetyCheckPassed()) {
            throw std::runtime_error("Safety check failed - operation blocked");
        }

        return func();
    }

    // 操作中的状态监控
    template<typename Func>
    auto withStateMonitoring(Func&& func) -> decltype(func()) {
        auto start_state = estop_manager_->getState();

        try {
            auto result = func();

            // 检查操作过程中是否触发了急停
            if (estop_manager_->getState() != start_state) {
                throw std::runtime_error("Emergency stop activated during operation");
            }

            return result;
        } catch (...) {
            // 如果发生异常，确保状态正确
            return withSafetyCheck([&]() { throw; });
        }
    }

private:
    std::shared_ptr<EmergencyStopManager> estop_manager_;
};

/**
 * @brief 急停保护（RAII模式）
 */
class EStopGuard {
public:
    EStopGuard(std::shared_ptr<EmergencyStopManager> estop_manager,
               const std::string& operation_name = "")
        : estop_manager_(estop_manager), operation_name_(operation_name) {
        if (estop_manager_) {
            estop_manager_->activate(EStopSource::SOFTWARE,
                                   "Operation: " + operation_name_);
        }
    }

    ~EStopGuard() {
        if (estop_manager_) {
            estop_manager_->reset();
        }
    }

    // 禁止拷贝
    EStopGuard(const EStopGuard&) = delete;
    EStopGuard& operator=(const EStopGuard&) = delete;

    // 允许移动
    EStopGuard(EStopGuard&&) = default;
    EStopGuard& operator=(EStopGuard&&) = default;

private:
    std::shared_ptr<EmergencyStopManager> estop_manager_;
    std::string operation_name_;
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_EMERGENCY_STOP_H