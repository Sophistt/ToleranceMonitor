#include "ToleranceChecker.h"
#include <iostream>
#include <cmath>

ToleranceChecker& ToleranceChecker::getInstance() {
    static ToleranceChecker instance;
    instance.startMonitoring();
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


void ToleranceChecker::startMonitoring() {
    if (m_isMonitoring.load()) {
        std::cout << "监控已经在运行中" << std::endl;
        return;
    }
    m_isMonitoring.store(true);
    m_monitoringThread = std::thread(&ToleranceChecker::monitoringLoop, this);

    std::cout << "开始监控，检查间隔: " << m_checkIntervalMs << "ms" << std::endl;
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
        return it->second.state;
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

void ToleranceChecker::checkSignal(const std::string& signalId, SignalInfo& sig) {
    auto now = std::chrono::steady_clock::now();
    
    // 获取当前信号值
    double currentValue = 0.0;
    try {
        currentValue = sig.config.valueCallback(signalId);
    } catch (const std::exception& e) {
        std::cerr << "获取信号 " << signalId << " 的值时发生错误: " << e.what() << std::endl;
        return;
    }
    
    // 检查tc等待期
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - sig.registrationTime).count();
    if (elapsedMs < sig.config.tcMs) {
        return;  // 仍在等待期
    }
    // 首次过等待期时输出日志
    if (sig.state == SignalState::UNKNOWN && elapsedMs >= sig.config.tcMs) {
        std::cout << "信号 " << signalId << " tc等待期结束，开始监控" << std::endl;
    }

    // 计算偏差值（当前值与目标值的差的绝对值）
    double deviation = std::abs(currentValue - sig.config.targetValue);
    
    // 1) 信号处于正常状态
    if (deviation <= sig.config.warningThreshold) {
        sig.state = SignalState::NORMAL;
        sig.warningTimerActive = sig.faultTimerActive = false;
        return;
    }
    
    // 2) 信号处于警告状态
    if (deviation <= sig.config.faultThreshold) {
        sig.faultTimerActive = false;
        if (!sig.warningTimerActive) {
            sig.warningTimerActive = true;
            sig.warningStartTime = now;
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - sig.warningStartTime).count()
            >= sig.config.tsMs) {
            if (sig.state != SignalState::WARNING && sig.config.warningCallback)
                sig.config.warningCallback(signalId, currentValue);
            sig.state = SignalState::WARNING;
        }
    }

    // 3) 信号处于故障状态
    else {
        if (!sig.faultTimerActive) {
            sig.faultStartTime = now;
            sig.faultTimerActive = true;
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - sig.faultStartTime).count()
            >= sig.config.tsMs) {
            if (sig.state != SignalState::FAULT && sig.config.faultCallback)
                sig.config.faultCallback(signalId, currentValue);
            sig.state = SignalState::FAULT;
        }
    }

}