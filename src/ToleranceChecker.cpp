#include "ToleranceChecker.h"
#include <iostream>
#include <cmath>

ToleranceChecker& ToleranceChecker::getInstance() {
    static ToleranceChecker instance;
    return instance;
}

ToleranceChecker::~ToleranceChecker() {
    stopMonitoring();
}

bool ToleranceChecker::registerSignal(const std::string& signalId, const SignalConfig& config) {
    std::lock_guard<std::mutex> lock(m_signalsMutex);
    
    if (m_signals.find(signalId) != m_signals.end()) {
        std::cerr << "信号 " << signalId << " 已经注册" << std::endl;
        return false;
    }
    
    auto result = m_signals.emplace(signalId, SignalInfo{});
    auto& signalInfo = result.first->second;
    signalInfo.config = config;
    signalInfo.registrationTime = std::chrono::steady_clock::now();
    
    std::cout << "信号 " << signalId << " 注册成功" << std::endl;
    return true;
}


void ToleranceChecker::startMonitoring(int checkIntervalMs) {
    if (m_isMonitoring.load()) {
        std::cout << "监控已经在运行中" << std::endl;
        return;
    }
    
    m_checkIntervalMs = checkIntervalMs;
    m_isMonitoring.store(true);
    
    m_monitoringThread = std::thread(&ToleranceChecker::monitoringLoop, this);
    
    std::cout << "开始监控，检查间隔: " << checkIntervalMs << "ms" << std::endl;
}

void ToleranceChecker::stopMonitoring() {
    if (!m_isMonitoring.load()) {
        return;
    }
    
    m_isMonitoring.store(false);
    
    if (m_monitoringThread.joinable()) {
        m_monitoringThread.join();
    }
    
    std::cout << "监控已停止" << std::endl;
}

bool ToleranceChecker::isMonitoring() const {
    return m_isMonitoring.load();
}

void ToleranceChecker::removeSignal(const std::string& signalId) {
    std::lock_guard<std::mutex> lock(m_signalsMutex);
    
    auto it = m_signals.find(signalId);
    if (it != m_signals.end()) {
        m_signals.erase(it);
        std::cout << "信号 " << signalId << " 已移除" << std::endl;
    }
}

SignalState ToleranceChecker::getSignalState(const std::string& signalId) const {
    std::lock_guard<std::mutex> lock(m_signalsMutex);
    
    auto it = m_signals.find(signalId);
    if (it != m_signals.end()) {
        return it->second.currentState;
    }
    
    return SignalState::NORMAL;
}

void ToleranceChecker::monitoringLoop() {
    while (m_isMonitoring.load()) {
        {
            std::lock_guard<std::mutex> lock(m_signalsMutex);
            
            for (auto& [signalId, signalInfo] : m_signals) {
                checkSignal(signalId, signalInfo);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(m_checkIntervalMs));
    }
}

void ToleranceChecker::checkSignal(const std::string& signalId, SignalInfo& signalInfo) {
    auto currentTime = std::chrono::steady_clock::now();
    
    // 获取当前信号值
    double currentValue = 0.0;
    try {
        currentValue = signalInfo.config.valueCallback(signalId);
    } catch (const std::exception& e) {
        std::cerr << "获取信号 " << signalId << " 的值时发生错误: " << e.what() << std::endl;
        return;
    }
    
    // 检查tc等待期
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - signalInfo.registrationTime).count();
    if (elapsedMs < signalInfo.config.tcMs) {
        return;  // 仍在等待期
    }
    // 首次过等待期时输出日志
    if (signalInfo.currentState == SignalState::UNKNOWN && elapsedMs >= signalInfo.config.tcMs) {
        std::cout << "信号 " << signalId << " tc等待期结束，开始监控" << std::endl;
    }

    // 计算偏差值（当前值与目标值的差的绝对值）
    double deviation = std::abs(currentValue - signalInfo.config.targetValue);
    
    // // 1) 信号处于正常状态
    // if (deviation <= signalInfo.config.warningThreshold) {
    //     signalInfo.currentState = SignalState::NORMAL;
    //     signalInfo.warningTimerActive = signalInfo.faultTimerActive = false;;
    //     return;
    // }
    
    // // 2) 信号处于警告状态
    // if (deviation <= signalInfo.config.faultThreshold) {
    //     signalInfo.currentState = SignalState::WARNING;
    //     if (!signalInfo.warningTimerActive) {
    //         signalInfo.warningTimerActive = true;
    //         signalInfo.warningStartTime = currentTime;
    //     }
    //     signalInfo.faultTimerActive = false;
    // }
    // // 3) 信号处于故障状态
    // else {
    //     if (!signalInfo.faultTimerActive) {
    //         signalInfo.faultStartTime = currentTime;
    //         signalInfo.faultTimerActive = true;
    //     }
    // }
    // 确定目标状态
    SignalState targetState = SignalState::NORMAL;
    if (deviation >= signalInfo.config.faultThreshold) {
        targetState = SignalState::FAULT;
    } else if (deviation >= signalInfo.config.warningThreshold) {
        targetState = SignalState::WARNING;
    }
    
    // 处理状态变化
    if (targetState != signalInfo.currentState) {
        handleStateTransition(signalId, signalInfo, targetState, currentValue, deviation);
    }
    
    // 检查并触发回调
    checkAndTriggerCallback(signalId, signalInfo, currentValue, currentTime);
}

void ToleranceChecker::checkAndTriggerCallback(const std::string& signalId, SignalInfo& signalInfo,
                                              double currentValue, const std::chrono::steady_clock::time_point& currentTime) {
    auto checkTimer = [&](bool& timerActive, const std::chrono::steady_clock::time_point& startTime,
                          const std::string& level, const std::function<void(const std::string&, double)>& callback) {
        if (!timerActive) return;
        
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime).count();
        
        if (elapsedMs >= signalInfo.config.tsMs) {
            timerActive = false;
            std::cout << "信号 " << signalId << " 触发" << level << "回调，值: " << currentValue << std::endl;
            if (callback) {
                callback(signalId, currentValue);
            }
        }
    };
    
    checkTimer(signalInfo.warningTimerActive, signalInfo.warningStartTime, "WARNING", signalInfo.config.warningCallback);
    checkTimer(signalInfo.faultTimerActive, signalInfo.faultStartTime, "FAULT", signalInfo.config.faultCallback);
}

void ToleranceChecker::handleStateTransition(const std::string& signalId, SignalInfo& signalInfo, 
                                           SignalState newState, double currentValue, double deviation) {
    auto currentTime = std::chrono::steady_clock::now();
    
    // 输出状态变化信息
    std::cout << "信号 " << signalId << " 状态变化: " << getStateName(signalInfo.currentState)
              << " -> " << getStateName(newState) 
              << " (当前值: " << currentValue 
              << ", 目标值: " << signalInfo.config.targetValue 
              << ", 偏差: " << deviation << ")" << std::endl;
    
    signalInfo.currentState = newState;
    
    // 根据新状态更新计时器
    switch (newState) {
        case SignalState::NORMAL:
            signalInfo.warningTimerActive = false;
            signalInfo.faultTimerActive = false;
            break;
            
        case SignalState::WARNING:
            if (!signalInfo.warningTimerActive) {
                signalInfo.warningStartTime = currentTime;
                signalInfo.warningTimerActive = true;
            }
            signalInfo.faultTimerActive = false;
            break;
            
        case SignalState::FAULT:
            if (!signalInfo.faultTimerActive) {
                signalInfo.faultStartTime = currentTime;
                signalInfo.faultTimerActive = true;
            }
            break;
            
        default:
            break;
    }
}

std::string ToleranceChecker::getStateName(SignalState state) const {
    switch (state) {
        case SignalState::UNKNOWN: return "UNKNOWN";
        case SignalState::NORMAL: return "NORMAL";
        case SignalState::WARNING: return "WARNING";
        case SignalState::FAULT: return "FAULT";
        default: return "INVALID";
    }
}