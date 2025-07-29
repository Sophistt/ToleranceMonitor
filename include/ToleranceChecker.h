#pragma once

#include <functional>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

// 信号状态枚举
enum class SignalState {
    UNKNOWN = 0,  // 初始未知状态
    NORMAL,
    WARNING,
    FAULT
};

// 回调函数类型定义
using WarningCallback = std::function<void(const std::string& signalId, double value)>;
using FaultCallback = std::function<void(const std::string& signalId, double value)>;
using ValueCallback = std::function<double(const std::string& signalId)>;

// 信号配置结构
struct SignalConfig {
    double targetValue;       // 目标值
    double warningThreshold;  // 容差警告阈值（偏差的绝对值）
    double faultThreshold;    // 容差故障阈值（偏差的绝对值）
    WarningCallback warningCallback;
    FaultCallback faultCallback;
    ValueCallback valueCallback;  // 获取信号值的回调函数
    int tcMs;  // 等待时间（毫秒）
    int tsMs;  // 持续时间（毫秒）
};

// 信号信息结构
struct SignalInfo {
    SignalConfig config;
    SignalState currentState{SignalState::UNKNOWN};  // 初始状态为UNKNOWN
    std::chrono::steady_clock::time_point registrationTime;
    std::chrono::steady_clock::time_point warningStartTime;
    std::chrono::steady_clock::time_point faultStartTime;
    bool warningTimerActive{false};
    bool faultTimerActive{false};
};

class ToleranceChecker {
public:
    static ToleranceChecker& getInstance();
    
    bool registerSignal(const std::string& signalId, const SignalConfig& config);
    
    void startMonitoring(int checkIntervalMs = 100);
    
    void stopMonitoring();
    
    bool isMonitoring() const;
    
    void removeSignal(const std::string& signalId);
    
    SignalState getSignalState(const std::string& signalId) const;

private:
    ToleranceChecker() = default;
    ~ToleranceChecker();
    
    ToleranceChecker(const ToleranceChecker&) = delete;
    ToleranceChecker& operator=(const ToleranceChecker&) = delete;
    ToleranceChecker(ToleranceChecker&&) = delete;
    ToleranceChecker& operator=(ToleranceChecker&&) = delete;

    void monitoringLoop();
    
    void checkSignal(const std::string& signalId, SignalInfo& signalInfo);
    
    void handleStateTransition(const std::string& signalId, SignalInfo& signalInfo, 
                              SignalState newState, double currentValue, double deviation);
    
    void checkAndTriggerCallback(const std::string& signalId, SignalInfo& signalInfo,
                                double currentValue, const std::chrono::steady_clock::time_point& currentTime);
    
    std::string getStateName(SignalState state) const;

private:
    mutable std::mutex m_signalsMutex;
    std::unordered_map<std::string, SignalInfo> m_signals;
    
    std::atomic<bool> m_isMonitoring{false};
    std::thread m_monitoringThread;
    int m_checkIntervalMs{100};
};