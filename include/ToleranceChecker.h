/**
 * @file ToleranceChecker.h
 * @brief 容差监控系统头文件
 * @author ToleranceMonitor Team
 * @version 1.0.0
 * @date 2024
 * 
 * 此头文件定义了容差监控系统的核心类和数据结构。
 * 系统支持多信号监控，具备时间控制机制和回调通知功能。
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

/**
 * @brief 信号状态枚举
 * 
 * 定义信号在监控过程中的四种可能状态：
 * - UNKNOWN: 初始未知状态（注册后、tc等待期内）
 * - NORMAL:  正常状态（偏差在警告阈值内）
 * - WARNING: 警告状态（偏差超过警告阈值但未超过故障阈值）
 * - FAULT:  故障状态（偏差超过故障阈值）
 */
enum class SignalState {
    UNKNOWN = 0,  ///< 初始未知状态，注册后tc等待期内的状态
    NORMAL,       ///< 正常状态，信号值在容差范围内
    WARNING,      ///< 警告状态，信号值超出警告阈值
    FAULT         ///< 故障状态，信号值超出故障阈值
};

/**
 * @brief 警告回调函数类型
 * @param signalId 信号标识符
 * @param value 触发警告时的信号值
 */
using WarningCallback = std::function<void(const std::string& signalId, double value)>;

/**
 * @brief 故障回调函数类型
 * @param signalId 信号标识符
 * @param value 触发故障时的信号值
 */
using FaultCallback = std::function<void(const std::string& signalId, double value)>;

/**
 * @brief 信号值获取回调函数类型
 * @param signalId 信号标识符
 * @return 当前信号值
 * 
 * 此回调用于实时获取信号值，支持动态数据源
 */
using ValueCallback = std::function<double(const std::string& signalId)>;

/**
 * @brief 信号配置结构
 * 
 * 包含信号监控所需的所有配置参数：
 * - 目标值和阈值设置
 * - 回调函数配置
 * - 时间控制参数
 */
struct SignalConfig {
    double targetValue;              ///< 信号目标值
    double warningThreshold;         ///< 警告阈值（与目标值偏差的绝对值）
    double faultThreshold;           ///< 故障阈值（与目标值偏差的绝对值）
    WarningCallback warningCallback; ///< 警告回调函数
    FaultCallback faultCallback;     ///< 故障回调函数
    ValueCallback valueCallback;     ///< 信号值获取回调函数
    int tcMs;                        ///< tc时间：注册后等待开始监控的时间（毫秒）
    int tsMs;                        ///< ts时间：超出阈值后持续监控时间（毫秒）
};

/**
 * @brief 信号信息结构（内部使用）
 * 
 * 存储信号的配置信息、当前状态和计时器状态
 * 此结构仅供ToleranceChecker内部使用
 */
struct SignalInfo {
    SignalConfig config;                                    ///< 信号配置
    SignalState  state{SignalState::UNKNOWN};               ///< 当前状态
    std::chrono::steady_clock::time_point registrationTime; ///< 注册时间点
    std::chrono::steady_clock::time_point warningStartTime; ///< 警告开始时间点
    std::chrono::steady_clock::time_point faultStartTime;   ///< 故障开始时间点
    bool warningTimerActive{false};                         ///< 警告计时器是否激活
    bool faultTimerActive{false};                           ///< 故障计时器是否激活
};

/**
 * @brief 容差监控器主类
 * 
 * 使用单例模式的线程安全容差监控系统。
 * 
 * 主要功能：
 * - 多信号注册和管理
 * - 实时监控和状态跟踪
 * - 基于时间的阈值检测
 * - 异步回调通知机制
 * 
 * 工作原理：
 * 1. 注册信号时进入UNKNOWN状态
 * 2. tc时间后开始监控，转为NORMAL状态
 * 3. 检测到偏差超过阈值时开始计时
 * 4. ts时间后触发相应回调并更新状态
 * 
 * 使用示例：
 * @code
 * auto& checker = ToleranceChecker::getInstance();
 * 
 * SignalConfig config;
 * config.targetValue = 25.0;
 * config.warningThreshold = 5.0;
 * config.faultThreshold = 10.0;
 * config.valueCallback = getSignalValue;
 * config.warningCallback = onWarning;
 * config.faultCallback = onFault;
 * config.tcMs = 1000;  // 1秒等待期
 * config.tsMs = 2000;  // 2秒持续期
 * 
 * checker.registerSignal("sensor_1", config);
 * // 监控自动开始
 * @endcode
 */
class ToleranceChecker {
public:
    /**
     * @brief 获取单例实例
     * @return ToleranceChecker的单例引用
     * 
     * 线程安全的单例获取方法，首次调用时创建实例并自动开始监控
     */
    static ToleranceChecker& getInstance();
    
    /**
     * @brief 注册信号
     * @param signalId 信号唯一标识符
     * @param config 信号配置结构
     * @return 成功返回true，失败返回false
     * 
     * 注册新的监控信号。注册后信号进入UNKNOWN状态，
     * tc时间后开始正式监控。
     * 
     * @note 相同signalId的信号不能重复注册
     */
    bool registerSignal(const std::string& signalId, const SignalConfig& config);
    
    /**
     * @brief 停止监控
     * 
     * 停止后台监控线程，不会删除已注册的信号。
     * 可以通过重新获取实例来重启监控。
     */
    void stopMonitoring();
    
    /**
     * @brief 检查监控状态
     * @return 监控中返回true，否则返回false
     */
    bool isMonitoring() const;
    
    /**
     * @brief 移除信号
     * @param signalId 要移除的信号标识符
     * 
     * 从监控系统中移除指定信号，释放相关资源
     */
    void removeSignal(const std::string& signalId);
    
    /**
     * @brief 获取信号当前状态
     * @param signalId 信号标识符
     * @return 信号当前状态，未找到信号时返回NORMAL
     */
    SignalState getSignalState(const std::string& signalId) const;

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    ToleranceChecker() = default;
    
    /**
     * @brief 析构函数
     * 自动停止监控线程
     */
    ~ToleranceChecker();
    
    // 禁用拷贝和移动操作（单例模式）
    ToleranceChecker(const ToleranceChecker&) = delete;            ///< 禁用拷贝构造
    ToleranceChecker& operator=(const ToleranceChecker&) = delete; ///< 禁用拷贝赋值
    ToleranceChecker(ToleranceChecker&&) = delete;                 ///< 禁用移动构造
    ToleranceChecker& operator=(ToleranceChecker&&) = delete;      ///< 禁用移动赋值

    /**
     * @brief 开始监控（内部方法）
     * 
     * 启动后台监控线程，开始周期性检查所有注册的信号
     */
    void startMonitoring();

    /**
     * @brief 监控主循环（内部方法）
     * 
     * 后台线程执行的主循环，周期性调用checkSignal检查所有信号
     */
    void monitoringLoop();
    
    /**
     * @brief 检查单个信号（内部方法）
     * @param signalId 信号标识符
     * @param signalInfo 信号信息引用
     * 
     * 检查单个信号的状态，包括：
     * - tc等待期检查
     * - 通过valueCallback获取当前值
     * - 计算偏差并判断状态
     * - 管理计时器和触发回调
     */
    void checkSignal(const std::string& signalId, SignalInfo& signalInfo);

private:
    mutable std::mutex m_signalsMutex;                    ///< 信号集合的互斥锁
    std::unordered_map<std::string, SignalInfo> m_signals; ///< 信号信息映射表
    
    std::atomic<bool> m_isMonitoring{false};              ///< 监控状态标志
    std::thread m_monitoringThread;                       ///< 后台监控线程
    int m_checkIntervalMs{100};                           ///< 检查间隔（毫秒）
};