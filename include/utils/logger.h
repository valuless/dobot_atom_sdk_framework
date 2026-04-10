/**
 * @file logger.h
 * @brief 日志记录器
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_LOGGER_H
#define DOBOT_ATOM_SDK_FRAMEWORK_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace robot_framework {

/**
 * @brief 日志级别
 */
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

/**
 * @brief 日志格式器
 */
class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual std::string format(LogLevel level, const std::string& message,
                             const std::string& file = "", int line = 0) = 0;
};

/**
 * @brief 默认日志格式器
 */
class DefaultLogFormatter : public LogFormatter {
public:
    std::string format(LogLevel level, const std::string& message,
                      const std::string& file = "", int line = 0) override;
};

/**
 * @brief 日志输出目标
 */
class LogAppender {
public:
    virtual ~LogAppender() = default;
    virtual void append(const std::string& formatted_message) = 0;
    virtual void setLogLevel(LogLevel level) { level_ = level; }
    LogLevel getLogLevel() const { return level_; }

protected:
    LogLevel level_ = LogLevel::DEBUG;
};

/**
 * @brief 控制台输出器
 */
class ConsoleAppender : public LogAppender {
public:
    void append(const std::string& formatted_message) override;
};

/**
 * @brief 文件输出器
 */
class FileAppender : public LogAppender {
public:
    FileAppender(const std::string& filename);
    ~FileAppender();
    void append(const std::string& formatted_message) override;

private:
    std::ofstream file_;
    std::mutex mutex_;
};

/**
 * @brief 日志记录器
 */
class Logger {
public:
    static Logger& getInstance();

    void setLevel(LogLevel level);
    void addAppender(std::shared_ptr<LogAppender> appender);
    void removeAppender(std::shared_ptr<LogAppender> appender);
    void setFormatter(std::shared_ptr<LogFormatter> formatter);

    // 记录日志
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const std::string& message, const std::string& file, int line);

    // 便捷宏
    #define LOG_TRACE(msg) Logger::getInstance().log(LogLevel::TRACE, msg, __FILE__, __LINE__)
    #define LOG_DEBUG(msg) Logger::getInstance().log(LogLevel::DEBUG, msg, __FILE__, __LINE__)
    #define LOG_INFO(msg) Logger::getInstance().log(LogLevel::INFO, msg, __FILE__, __LINE__)
    #define LOG_WARN(msg) Logger::getInstance().log(LogLevel::WARN, msg, __FILE__, __LINE__)
    #define LOG_ERROR(msg) Logger::getInstance().log(LogLevel::ERROR, msg, __FILE__, __LINE__)
    #define LOG_FATAL(msg) Logger::getInstance().log(LogLevel::FATAL, msg, __FILE__, __LINE__)

    // 条件日志
    #if defined(DEBUG) || defined(_DEBUG)
    #define LOG_IF(level, condition) if (condition) LOG_##level
    #else
    #define LOG_IF(level, condition) if (false) LOG_##level
    #endif

private:
    Logger();
    ~Logger();

    std::mutex mutex_;
    LogLevel level_ = LogLevel::INFO;
    std::vector<std::shared_ptr<LogAppender>> appenders_;
    std::shared_ptr<LogFormatter> formatter_;
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_LOGGER_H