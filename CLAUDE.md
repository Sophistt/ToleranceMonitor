# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ToleranceMonitor是一个C++17实现的容差监控系统，提供信号监控、阈值检测和回调通知功能。主要用于工业监控、质量控制等场景。

### 核心特性
- **单例模式监控器**：ToleranceChecker使用线程安全的单例模式
- **回调函数机制**：通过回调函数实时获取信号值，无需存储数据
- **多状态管理**：支持UNKNOWN、NORMAL、WARNING、FAULT四种状态
- **时间控制**：支持延迟开始监控(tc)和持续时间判断(ts)
- **模块化设计**：数据结构与监控逻辑分离

## 项目结构

```
ToleranceMonitor/
├── CMakeLists.txt          # CMake构建配置
├── include/
│   └── ToleranceChecker.h  # 核心单例类头文件
├── src/
│   ├── ToleranceChecker.cpp # 单例类实现
│   └── main.cpp            # 演示程序
└── CLAUDE.md              # 项目文档
```

## 构建命令

```bash
# 创建构建目录
mkdir build && cd build

# 配置构建
cmake ..

# 编译项目
make

# 运行程序
./bin/ToleranceMonitor
```

## 核心架构

### 核心数据结构（类外定义）

#### SignalState 枚举
```cpp
enum class SignalState {
    UNKNOWN = 0,  // 初始未知状态（刚注册时）
    NORMAL,       // 正常状态
    WARNING,      // 警告状态
    FAULT         // 故障状态
};
```

#### SignalConfig 结构
```cpp
struct SignalConfig {
    double warningThreshold;              // 警告阈值
    double faultThreshold;                // 故障阈值
    WarningCallback warningCallback;      // 警告回调函数
    FaultCallback faultCallback;          // 故障回调函数
    ValueCallback valueCallback;          // 获取信号值的回调函数
    int tcMs;                            // 等待时间（毫秒）
    int tsMs;                            // 持续时间（毫秒）
};
```

#### SignalInfo 结构
包含信号的配置、当前状态和计时器信息。

### ToleranceChecker 单例类
- **单例模式**：确保全局唯一的监控实例
- **线程安全**：使用互斥锁保护共享数据
- **信号管理**：支持注册、移除信号
- **异步监控**：后台线程定期检查信号状态
- **回调机制**：通过valueCallback实时获取信号值

### 信号监控逻辑
1. **时间控制**：
   - `tc`毫秒：注册后等待时间，之后开始监控
   - `ts`毫秒：超出阈值持续时间，达到后触发回调

2. **状态转换**：
   - 注册时：`UNKNOWN`状态
   - tc等待期结束后：`UNKNOWN` → `NORMAL/WARNING/FAULT`
   - 运行时转换：`NORMAL` ↔ `WARNING` ↔ `FAULT`
   - fault→warning时重置fault计时器
   - 低于warning时重置所有计时器

3. **阈值检测**：
   - 通过valueCallback实时获取信号值
   - 与warningThreshold和faultThreshold比较
   - 持续超出阈值ts毫秒后触发对应回调

## 关键API

### 回调函数类型
```cpp
using WarningCallback = std::function<void(const std::string& signalId, double value)>;
using FaultCallback = std::function<void(const std::string& signalId, double value)>;
using ValueCallback = std::function<double(const std::string& signalId)>;
```

### 使用示例
```cpp
// 全局变量存储传感器数据
double g_temperature = 20.0;

// 实现回调函数
double getTemperatureValue(const std::string& signalId) {
    return g_temperature;  // 返回实时传感器值
}

void onTemperatureWarning(const std::string& signalId, double value) {
    std::cout << "警告: " << signalId << " = " << value << std::endl;
}

// 获取单例实例
auto& checker = ToleranceChecker::getInstance();

// 配置并注册信号
SignalConfig config;
config.warningThreshold = 60.0;
config.faultThreshold = 80.0;
config.warningCallback = onTemperatureWarning;
config.faultCallback = onTemperatureFault;
config.valueCallback = getTemperatureValue;  // 关键：提供值获取回调
config.tcMs = 1000;  // 1秒等待期
config.tsMs = 2000;  // 2秒持续期

checker.registerSignal("temp_sensor", config);

// 开始监控
checker.startMonitoring(100);  // 100ms检查间隔

// 在业务逻辑中更新数据（不再需要调用updateSignalValue）
g_temperature = 75.0;  // 直接修改数据源

// 停止监控
checker.stopMonitoring();
```

## 编译要求

- C++17标准支持
- CMake 3.12+
- pthread库支持
- 支持std::thread, std::mutex, std::atomic等特性