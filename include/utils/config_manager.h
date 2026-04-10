/**
 * @file config_manager.h
 * @brief 配置管理器
 * @author Framework
 * @date 2024
 */

#ifndef DOBOT_ATOM_SDK_FRAMEWORK_CONFIG_MANAGER_H
#define DOBOT_ATOM_SDK_FRAMEWORK_CONFIG_MANAGER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "../nlohmann/json.hpp"

namespace robot_framework {

/**
 * @brief 配置管理器类
 */
class ConfigManager {
public:
    using Json = nlohmann::json;

    // 单例模式
    static ConfigManager& getInstance();

    // 基本操作
    bool loadConfig(const std::string& config_path);
    bool saveConfig(const std::string& config_path = "") const;
    bool reloadConfig();

    // 获取配置值
    template<typename T>
    T getValue(const std::string& key, const T& default_value = T{}) const;

    // 设置配置值
    template<typename T>
    void setValue(const std::string& key, const T& value);

    // 批量操作
    Json getSection(const std::string& section) const;
    void setSection(const std::string& section, const Json& values);
    void removeSection(const std::string& section);

    // 配置验证
    bool validateConfig() const;
    std::vector<std::string> getValidationErrors() const;

    // 配置监听
    using ConfigCallback = std::function<void(const std::string& key, const Json& old_value, const Json& new_value)>;
    void addListener(const std::string& key, ConfigCallback callback);
    void removeListener(const std::string& key);

private:
    ConfigManager();
    ~ConfigManager();

    // 配置文件操作
    Json loadFromFile(const std::string& path) const;
    bool saveToFile(const std::string& path, const Json& config) const;

    // 配置变更通知
    void notifyListeners(const std::string& key, const Json& old_value, const Json& new_value);

private:
    std::mutex mutex_;
    Json config_;
    Json default_config_;
    std::string config_path_;
    std::unordered_map<std::string, std::vector<ConfigCallback>> listeners_;
};

// 便捷函数
template<typename T>
T ConfigGet(const std::string& key, const T& default_value = T{}) {
    return ConfigManager::getInstance().getValue<T>(key, default_value);
}

template<typename T>
void ConfigSet(const std::string& key, const T& value) {
    ConfigManager::getInstance().setValue<T>(key, value);
}

} // namespace robot_framework

#endif // DOBOT_ATOM_SDK_FRAMEWORK_CONFIG_MANAGER_H