#include "ToleranceChecker.h"
#include <iostream>
#include <thread>
#include <chrono>

// 全局变量存储传感器数据
double g_temperature = 20.0;
double g_pressure = 1000.0;

// 信号值回调函数
double getTemperatureValue(const std::string& signalId) {
    return g_temperature;
}

double getPressureValue(const std::string& signalId) {
    return g_pressure;
}

// 警告和故障回调函数
void onTemperatureWarning(const std::string& signalId, double value) {
    std::cout << "🟡 温度警告！信号: " << signalId << ", 当前值: " << value << "°C" << std::endl;
}

void onTemperatureFault(const std::string& signalId, double value) {
    std::cout << "🔴 温度故障！信号: " << signalId << ", 当前值: " << value << "°C" << std::endl;
}

void onPressureWarning(const std::string& signalId, double value) {
    std::cout << "🟡 压力警告！信号: " << signalId << ", 当前值: " << value << " Pa" << std::endl;
}

void onPressureFault(const std::string& signalId, double value) {
    std::cout << "🔴 压力故障！信号: " << signalId << ", 当前值: " << value << " Pa" << std::endl;
}

void simulateTemperatureSensor() {
    // 模拟温度变化
    std::vector<double> temperatures = {
        20.0,   // 正常
        25.0,   // 正常  
        65.0,   // 警告阈值
        70.0,   // 继续警告
        75.0,   // 继续警告
        85.0,   // 故障阈值
        90.0,   // 继续故障
        95.0,   // 继续故障
        70.0,   // 回到警告
        60.0,   // 回到警告
        30.0,   // 回到正常
        25.0    // 继续正常
    };
    
    for (size_t i = 0; i < temperatures.size(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        g_temperature = temperatures[i];  // 直接更新全局变量
        std::cout << "温度传感器更新: " << temperatures[i] << "°C" << std::endl;
    }
}

void simulatePressureSensor() {
    // 模拟压力变化
    std::vector<double> pressures = {
        1000.0,  // 正常
        1200.0,  // 正常
        1400.0,  // 正常
        1600.0,  // 警告阈值
        1700.0,  // 继续警告
        1900.0,  // 故障阈值
        2100.0,  // 继续故障
        1500.0,  // 回到警告
        1000.0,  // 回到正常
        800.0    // 继续正常
    };
    
    for (size_t i = 0; i < pressures.size(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        g_pressure = pressures[i];  // 直接更新全局变量
        std::cout << "压力传感器更新: " << pressures[i] << " Pa" << std::endl;
    }
}

int main() {
    std::cout << "=== ToleranceMonitor 演示程序 ===" << std::endl;
    std::cout << std::endl;
    
    auto& checker = ToleranceChecker::getInstance();
    
    // 注册温度传感器信号
    SignalConfig tempConfig;
    tempConfig.targetValue = 40.0;       // 目标温度40°C
    tempConfig.warningThreshold = 20.0;  // 偏差±20°C触发警告
    tempConfig.faultThreshold = 40.0;    // 偏差±40°C触发故障
    tempConfig.warningCallback = onTemperatureWarning;
    tempConfig.faultCallback = onTemperatureFault;
    tempConfig.valueCallback = getTemperatureValue;  // 添加值获取回调
    tempConfig.tcMs = 1000;  // 等待1秒后开始监控
    tempConfig.tsMs = 2000;  // 持续2秒后触发回调
    
    checker.registerSignal("temperature_sensor", tempConfig);
    
    // 注册压力传感器信号
    SignalConfig pressureConfig;
    pressureConfig.targetValue = 1200.0;       // 目标压力1200Pa
    pressureConfig.warningThreshold = 300.0;   // 偏差±300Pa触发警告
    pressureConfig.faultThreshold = 600.0;     // 偏差±600Pa触发故障
    pressureConfig.warningCallback = onPressureWarning;
    pressureConfig.faultCallback = onPressureFault;
    pressureConfig.valueCallback = getPressureValue;  // 添加值获取回调
    pressureConfig.tcMs = 1500;  // 等待1.5秒后开始监控
    pressureConfig.tsMs = 1500;  // 持续1.5秒后触发回调
    
    checker.registerSignal("pressure_sensor", pressureConfig);
    
    // 开始监控
    checker.startMonitoring(100);  // 每100ms检查一次
    
    std::cout << std::endl;
    std::cout << "开始传感器数据模拟..." << std::endl;
    std::cout << "温度：目标值=40°C，警告容差=±20°C，故障容差=±40°C" << std::endl;
    std::cout << "压力：目标值=1200Pa，警告容差=±300Pa，故障容差=±600Pa" << std::endl;
    std::cout << std::endl;
    
    // 启动传感器模拟线程
    std::thread tempThread(simulateTemperatureSensor);
    std::thread pressureThread(simulatePressureSensor);
    
    // 等待模拟完成
    tempThread.join();
    pressureThread.join();
    
    // 等待一段时间让所有回调触发
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // 停止监控
    checker.stopMonitoring();
    
    std::cout << std::endl;
    std::cout << "=== 演示程序结束 ===" << std::endl;
    
    return 0;
}