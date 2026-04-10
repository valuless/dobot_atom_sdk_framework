# Dobot Atom SDK Framework

一个面向Dobot Atom机器人的高级开发框架，提供完整的机器人控制、状态监控和安全管理功能。

## 📋 目录

- [特性](#特性)
- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [模块说明](#模块说明)
- [示例程序](#示例程序)
- [API文档](#api文档)
- [配置说明](#配置说明)
- [构建和安装](#构建和安装)
- [最佳实践](#最佳实践)
- [故障排除](#故障排除)

## 🎯 特性

### 核心功能
- **统一的机器人接口** - 高度抽象的API，支持多种机器人型号
- **运动控制系统** - 支持关节空间和笛卡尔空间运动规划
- **实时状态监控** - 高频状态采集和实时数据处理
- **安全管理系统** - 多层次的安全规则和急停管理
- **配置管理** - 灵活的配置系统，支持热重载
- **日志系统** - 多级日志记录，支持文件和控制台输出

### 高级特性
- **异步控制** - 支持异步命令执行和回调
- **轨迹平滑** - 多种轨迹生成算法
- **安全规则引擎** - 可扩展的安全规则系统
- **数据记录** - 支持CSV格式的数据记录
- **RAII模式** - 自动资源管理，防止内存泄漏

## 🚀 快速开始

### 环境要求

- Linux (Ubuntu 18.04+ 推荐)
- GCC 7.0+ 或 Clang 6.0+
- CMake 3.16+
- 网络连接

### 安装依赖

```bash
# 安装基础依赖
sudo apt update
sudo apt install -y cmake build-essential

# 安装第三方库
sudo apt install -y libcyclonedds-dev libyaml-cpp-dev libeigen3-dev
```

### 编译框架

```bash
# 克隆或下载框架代码
cd dobot_atom_sdk_framework

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ..
make -j$(nproc)
```

### 运行示例

```bash
# 运行基础示例
./bin/examples/hello_robot

# 运动规划演示
./bin/examples/motion_planning_demo

# 安全监控演示
./bin/examples/safety_monitoring_demo
```

## 📁 项目结构

```
dobot_atom_sdk_framework/
├── include/                    # 头文件
│   ├── robot/                 # 机器人接口
│   │   ├── robot_interface.h
│   │   └── dobot_robot.h
│   ├── control/               # 运动控制
│   │   ├── motion_types.h
│   │   └── motion_controller.h
│   ├── monitor/               # 状态监控
│   │   ├── state_monitor.h
│   │   └── data_recorder.h
│   ├── safety/                # 安全管理
│   │   ├── safety_manager.h
│   │   └── emergency_stop.h
│   └── utils/                 # 工具类
│       ├── config_manager.h
│       └── logger.h
├── src/                       # 源文件
│   └── ... (与include目录对应)
├── examples/                  # 示例程序
│   ├── basic/                 # 基础示例
│   └── advanced/              # 高级示例
├── config/                    # 配置文件
│   └── robot/                 # 机器人配置
├── tests/                     # 测试代码
├── CMakeLists.txt             # 构建配置
└── README.md                  # 本文件
```

## 🔧 模块说明

### 机器人模块 (robot)

#### RobotInterface
抽象基类，定义了机器人的基本接口：
- 连接管理
- 状态查询
- 安全控制
- 系统信息

#### DobotRobot
具体实现类，提供：
- RPC和DDS双通信支持
- 状态监控集成
- 运动控制接口
- 安全系统管理

### 运动控制模块 (control)

#### MotionTypes
定义了运动相关的数据类型：
- JointPoint - 关节空间点
- CartesianPoint - 笛卡尔空间点
- Trajectory - 轨迹结构
- MotionCommand - 运动命令

#### MotionController
运动控制器核心类：
- 运动命令队列管理
- 异步执行支持
- 安全检查集成
- 轨迹规划接口

#### SimpleTrajectoryGenerator
轨迹生成器实现：
- 线性插值
- 平滑处理
- 多点轨迹规划

### 状态监控模块 (monitor)

#### StateMonitor
状态监控器：
- 高频数据采集（可配置频率）
- 实时状态分析
- 历史数据存储
- 回调通知机制

#### DataRecorder
数据记录器：
- CSV格式数据记录
- 可配置采样率
- 事件日志记录
- 多种格式支持（可选）

### 安全管理模块 (safety)

#### SafetyManager
安全管理器：
- 多规则引擎
- 事件管理
- 统计分析
- 安全状态查询

#### SafetyRule
安全规则基类：
- 可扩展的规则系统
- 规则配置管理
- 事件触发机制

#### EmergencyStopManager
急停管理器：
- 多源急停支持
- 自动复位功能
- 状态跟踪
- 安全检查集成

### 工具模块 (utils)

#### ConfigManager
配置管理器：
- JSON配置文件支持
- 热重载功能
- 配置验证
- 配置变更通知

#### Logger
日志系统：
- 多级日志（TRACE, DEBUG, INFO, WARN, ERROR, FATAL）
- 多种输出目标（控制台、文件）
- 线程安全
- 格式化输出

## 💡 示例程序

### 基础示例 (hello_robot)

```cpp
#include "robot/dobot_robot.h"
#include <iostream>

int main() {
    // 创建机器人实例
    auto robot = RobotFactory::createRobot();
    
    // 连接机器人
    robot->connect("192.168.8.234", 50051);
    
    // 设置状态回调
    robot->setStateCallback([](RobotState state) {
        std::cout << "Robot state changed to: " << (int)state << std::endl;
    });
    
    // 执行简单运动
    JointPoint target;
    target.positions = {0.5, 0.3, -0.8, 0.0, 0.0, 0.0};
    MotionCommand command;
    command.type = MotionCommand::Type::MOVE_JOINTS;
    command.joint_point = target;
    
    robot->executeMotion(command);
    
    // 断开连接
    robot->disconnect();
    
    return 0;
}
```

### 运动规划示例 (motion_planning_demo)

```cpp
#include "control/motion_controller.h"
#include <vector>

int main() {
    // 创建运动控制器
    MotionController controller;
    
    // 定义轨迹点
    std::vector<JointPoint> waypoints;
    // ... 添加轨迹点
    
    // 生成轨迹
    Trajectory trajectory;
    auto generator = std::make_shared<SimpleTrajectoryGenerator>();
    generator->generateTrajectory(waypoints, trajectory);
    
    // 执行轨迹
    MotionCommand command;
    command.type = MotionCommand::Type::EXECUTE_TRAJECTORY;
    command.trajectory = trajectory;
    
    auto future = controller.executeCommand(command);
    future.wait();  // 等待完成
    
    return 0;
}
```

### 安全监控示例 (safety_monitoring_demo)

```cpp
#include "safety/safety_manager.h"
#include "monitor/state_monitor.h"

int main() {
    // 创建机器人实例
    auto robot = RobotFactory::createRobot();
    robot->connect("192.168.8.234", 50051);
    
    // 创建状态监控器
    StateMonitor monitor(robot.get());
    monitor.start();
    
    // 创建安全管理器
    auto safety_manager = SafetyManagerFactory::createComprehensiveSafetyManager(&monitor);
    safety_manager->start();
    
    // 设置事件回调
    safety_manager->setEventCallback([](const SafetyEvent& event) {
        std::cout << "Safety event: " << event.message << std::endl;
    });
    
    // 运行监控...
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    // 清理
    safety_manager->stop();
    monitor.stop();
    robot->disconnect();
    
    return 0;
}
```

## 📚 API文档

### 核心类使用

#### 1. 创建和连接机器人

```cpp
// 创建机器人实例
auto robot = RobotFactory::createRobot();

// 连接到机器人
bool connected = robot->connect("192.168.8.234", 50051);

// 检查连接状态
if (robot->isConnected()) {
    std::cout << "Connected successfully!" << std::endl;
}
```

#### 2. 执行运动命令

```cpp
// 关节运动
MotionCommand joint_cmd;
joint_cmd.type = MotionCommand::Type::MOVE_JOINTS;
joint_cmd.joint_point.positions = {0.5, 0.3, -0.8, 0.0, 0.0, 0.0};

// 笛卡尔运动
MotionCommand cartesian_cmd;
cartesian_cmd.type = MotionCommand::Type::MOVE_CARTESIAN;
cartesian_cmd.cartesian_point.position = {0.3, 0.0, 0.5};

// 速度控制
MotionCommand velocity_cmd;
velocity_cmd.type = MotionCommand::Type::VELOCITY_CMD;
velocity_cmd.velocity_cmd.linear_vel = {0.1, 0.0, 0.0};
velocity_cmd.velocity_cmd.duration = 2.0;

// 执行命令
robot->executeMotion(joint_cmd);
```

#### 3. 安全管理

```cpp
// 检查安全状态
if (robot->isSafeToOperate()) {
    // 执行安全操作
}

// 急停控制
robot->emergencyStop("Manual E-Stop");
robot->resetEmergencyStop();

// 急停保护
EStopGuard estop_guard(estop_manager, "Protected Operation");
// 执行受保护的操作
```

#### 4. 状态监控

```cpp
// 设置状态回调
monitor.setStateCallback([](const RobotStateData& state) {
    std::cout << "FSM: " << state.fsm_id << std::endl;
    
    // 检查关节温度
    for (const auto& [name, joint] : state.joints) {
        if (joint.temperature > 60.0) {
            std::cout << "Warning: " << name << " hot!" << std::endl;
        }
    }
});

// 启动监控
monitor.start();
```

## ⚙️ 配置说明

### 机器人配置 (config/robot/default_robot_config.json)

```json
{
    "robot": {
        "ip": "192.168.8.234",
        "port": 50051,
        "timeout": 5.0
    },
    "motion": {
        "control_frequency": 100.0,
        "max_joint_velocity": 2.0,
        "max_joint_acceleration": 4.0
    },
    "monitoring": {
        "update_frequency": 50.0,
        "joint_temperature_threshold": 60.0
    },
    "safety": {
        "emergency_stop_timeout": 1.0,
        "safety_margin": 0.05
    }
}
```

### 使用配置管理器

```cpp
// 加载配置
ConfigManager& config = ConfigManager::getInstance();
config.loadConfig("config/robot/default_robot_config.json");

// 读取配置值
std::string ip = config.getValue<std::string>("robot.ip", "192.168.8.234");
double timeout = config.getValue<double>("robot.timeout", 5.0);

// 设置配置值
config.setValue("safety.margin", 0.1);

// 监听配置变更
config.addListener("robot.ip", [](const std::string& key, 
                                const Json& old_value, const Json& new_value) {
    std::cout << "IP changed from " << old_value 
              << " to " << new_value << std::endl;
});
```

## 🔨 构建和安装

### 开发模式构建

```bash
# 创建构建目录
mkdir build && cd build

# Debug模式构建
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# 运行测试
make test

# 启用调试信息
cmake -DENABLE_DEBUG=ON ..
```

### 安装

```bash
# 安装到系统
sudo make install

# 安装开发包（包含头文件）
sudo make install-dev
```

### 生成文档

```bash
# 需要先安装Doxygen
sudo apt install doxygen

# 生成文档
cmake -DBUILD_DOCUMENTATION=ON ..
make doc

# 文档将在 build/html/ 目录中
```

## 💡 最佳实践

### 1. 资源管理

```cpp
// 使用智能指针管理机器人实例
std::shared_ptr<RobotInterface> robot = RobotFactory::createRobot();

// 使用RAII模式管理连接
class RobotConnection {
    std::shared_ptr<RobotInterface> robot_;
public:
    RobotConnection(const std::string& ip, int port) 
        : robot_(RobotFactory::createRobot()) {
        robot_->connect(ip, port);
    }
    ~RobotConnection() {
        robot_->disconnect();
    }
    // ... 其他方法
};
```

### 2. 错误处理

```cpp
try {
    if (!robot->connect(ip, port)) {
        throw std::runtime_error("Connection failed");
    }
    
    // 执行操作...
} catch (const std::exception& e) {
    LOG_ERROR("Error: %s", e.what());
    // 清理资源...
}
```

### 3. 线程安全

```cpp
// 使用互斥锁保护共享资源
std::mutex state_mutex_;
RobotState current_state_;

void updateState(RobotState state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    current_state_ = state;
}

RobotState getState() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_state_;
}
```

### 4. 性能优化

```cpp
// 使用对象池减少内存分配
class MotionCommandPool {
    std::vector<MotionCommand> pool_;
    std::mutex mutex_;
public:
    MotionCommand* acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.empty()) {
            return new MotionCommand();
        }
        auto cmd = pool_.back();
        pool_.pop_back();
        return &cmd;
    }
    
    void release(MotionCommand* cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push_back(*cmd);
    }
};
```

## 🔧 故障排除

### 常见问题

#### 1. 连接失败

```bash
# 检查网络连接
ping 192.168.8.234

# 检查端口
netstat -an | grep 50051

# 检查防火墙
sudo ufw status
```

#### 2. 权限问题

```bash
# 确保有执行权限
chmod +x bin/examples/*

# 检查库文件权限
sudo ldconfig
```

#### 3. 依赖问题

```bash
# 检查CycloneDDS
pkg-config --libs cyclonedds

# 检查YAML-CPP
pkg-config --libs yaml-cpp

# 如果找不到，手动指定路径
cmake -DCYCLONEDDS_INCLUDE_DIR=/usr/include ..
```

### 调试技巧

#### 1. 启用详细日志

```cpp
Logger::getInstance().setLevel(LogLevel::DEBUG);

// 在代码中添加调试信息
LOG_DEBUG("Current joint position: [%.2f, %.2f, ...]",
          joints[0], joints[1]);
```

#### 2. 使用gdb调试

```bash
# 编译时启用调试信息
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# 使用gdb调试
gdb ./bin/examples/hello_robot

(gdb) break main
(gdb) run
(gdb) info threads
(gdb) thread apply all bt
```

#### 3. 性能分析

```bash
# 使用perf分析性能
perf record -g ./bin/examples/motion_planning_demo
perf report

# 使用valgrind检查内存泄漏
valgrind --leak-check=full ./bin/examples/hello_robot
```

## 📄 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 📞 支持

如有问题，请：
1. 查看 [故障排除](#故障排除) 章节
2. 搜索现有的 Issue
3. 创建新的 Issue 并提供详细信息

---

**注意**: 在使用本框架前，请确保您已经熟悉Dobot Atom机器人的基本操作和安全注意事项。