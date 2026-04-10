/**
 * @file data_recorder.h
 * @brief 数据记录器
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_DATA_RECORDER_H
#define DOBOT_ATOM_SDK_FRAMEWORK_DATA_RECORDER_H

#include "state_monitor.h"
#include <fstream>
#include <string>
#include <memory>
#include <functional>

namespace robot_framework {

/**
 * @brief 数据记录器接口
 */
class DataRecorder {
public:
    virtual ~DataRecorder() = default;

    // 记录控制
    virtual bool startRecording(const std::string& session_name) = 0;
    virtual bool stopRecording() = 0;
    virtual bool isRecording() const = 0;

    // 数据记录
    virtual bool recordData(const RobotStateData& data) = 0;
    virtual bool recordEvent(const std::string& event, const std::string& details = "") = 0;

    // 配置
    virtual void setSamplingRate(double rate) = 0;
    virtual double getSamplingRate() const = 0;
    virtual void setFileFormat(const std::string& format) = 0;
};

/**
 * @brief CSV格式记录器
 */
class CsvDataRecorder : public DataRecorder {
public:
    CsvDataRecorder();
    ~CsvDataRecorder();

    // 记录控制
    bool startRecording(const std::string& session_name) override;
    bool stopRecording() override;
    bool isRecording() const override;

    // 数据记录
    bool recordData(const RobotStateData& data) override;
    bool recordEvent(const std::string& event, const std::string& details = "") override;

    // 配置
    void setSamplingRate(double rate) override;
    double getSamplingRate() const override;
    void setFileFormat(const std::string& format) override;

    // 特殊功能
    void setCustomColumns(const std::vector<std::string>& columns);
    void enableTimestampColumn(bool enable);
    void enableEventLogging(bool enable);

private:
    std::string generateFilename(const std::string& session_name) const;
    void writeHeader();
    void writeData(const RobotStateData& data);
    void writeEvent(const std::string& event, const std::string& details);

private:
    std::ofstream file_;
    std::string session_name_;
    bool is_recording_ = false;
    double sampling_rate_ = 100.0;
    std::string file_format_ = "csv";
    std::vector<std::string> custom_columns_;
    bool enable_timestamp_ = true;
    bool enable_events_ = true;
    std::mutex mutex_;
};

/**
 * @brief ROS Bag格式记录器（可选）
 */
#ifdef ROS_AVAILABLE
class RosbagDataRecorder : public DataRecorder {
public:
    RosbagDataRecorder();
    ~RosbagDataRecorder();

    bool startRecording(const std::string& session_name) override;
    bool stopRecording() override;
    bool isRecording() const override;

    bool recordData(const RobotStateData& data) override;
    bool recordEvent(const std::string& event, const std::string& details = "") override;

    void setSamplingRate(double rate) override;
    double getSamplingRate() const override;
    void setFileFormat(const std::string& format) override;

private:
    // ROS相关实现
    void* rosbag_handle_;
    std::string bag_filename_;
    bool is_recording_;
    double sampling_rate_;
};
#endif

/**
 * @brief 数据记录器工厂
 */
class DataRecorderFactory {
public:
    static std::unique_ptr<DataRecorder> createRecorder(const std::string& type);
    static std::unique_ptr<DataRecorder> createDefaultRecorder();
};

/**
 * @brief 数据记录会话管理器
 */
class RecordingSession {
public:
    RecordingSession(std::unique_ptr<DataRecorder> recorder);
    ~RecordingSession();

    // 会话控制
    bool start(const std::string& session_name);
    bool stop();
    bool isActive() const;

    // 数据记录
    bool record(const RobotStateData& data);
    bool logEvent(const std::string& event, const std::string& details = "");

    // 会话信息
    std::string getSessionName() const;
    std::string getRecordingPath() const;
    std::chrono::system_clock::time_point getStartTime() const;
    std::chrono::system_clock::time_point getEndTime() const;
    double getDuration() const;

    // 统计信息
    struct SessionStats {
        size_t total_samples;
        size_t total_events;
        double avg_sampling_rate;
        std::string file_size;
    };

    SessionStats getStatistics() const;

private:
    std::unique_ptr<DataRecorder> recorder_;
    std::string session_name_;
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point end_time_;
    bool active_;
};

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_DATA_RECORDER_H