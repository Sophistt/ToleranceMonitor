#include "ToleranceChecker_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep

// 全局变量存储传感器数据
static double g_temperature = 20.0;
static double g_pressure = 100.0;

// 实现获取传感器值的回调函数
double get_temperature_value(const char* signal_id) {
    printf("[C Demo] 获取温度传感器值: %s = %.2f\n", signal_id, g_temperature);
    return g_temperature;
}

double get_pressure_value(const char* signal_id) {
    printf("[C Demo] 获取压力传感器值: %s = %.2f\n", signal_id, g_pressure);
    return g_pressure;
}

// 实现警告回调函数
void on_temperature_warning(const char* signal_id, double value) {
    printf("[C Demo] ⚠️  温度警告: %s = %.2f\n", signal_id, value);
}

void on_temperature_fault(const char* signal_id, double value) {
    printf("[C Demo] ❌ 温度故障: %s = %.2f\n", signal_id, value);
}

void on_pressure_warning(const char* signal_id, double value) {
    printf("[C Demo] ⚠️  压力警告: %s = %.2f\n", signal_id, value);
}

void on_pressure_fault(const char* signal_id, double value) {
    printf("[C Demo] ❌ 压力故障: %s = %.2f\n", signal_id, value);
}

// 打印信号状态
void print_signal_state(const char* signal_id) {
    tc_signal_state_t state;
    int result = tc_get_signal_state(signal_id, &state);
    if (result == TC_SUCCESS) {
        printf("[C Demo] 信号 %s 当前状态: %s\n", signal_id, tc_get_state_name(state));
    } else {
        printf("[C Demo] 获取信号 %s 状态失败，错误码: %d\n", signal_id, result);
    }
}

// 模拟传感器数据变化
void simulate_sensor_data_changes(void) {
    printf("\n[C Demo] === 开始模拟传感器数据变化 ===\n");
    
    // 正常状态
    printf("\n[C Demo] 1. 设置正常值\n");
    g_temperature = 25.0;  // 目标值25，在容差范围内
    g_pressure = 105.0;    // 目标值100，在容差范围内
    sleep(1);
    print_signal_state("temp_sensor");
    print_signal_state("pressure_sensor");
    
    // 警告状态
    printf("\n[C Demo] 2. 设置警告级别的值\n");
    g_temperature = 35.0;  // 偏差10，超过警告阈值8
    g_pressure = 115.0;    // 偏差15，超过警告阈值12
    sleep(3);  // 等待超过ts时间(2秒)以触发回调
    print_signal_state("temp_sensor");
    print_signal_state("pressure_sensor");
    
    // 故障状态
    printf("\n[C Demo] 3. 设置故障级别的值\n");
    g_temperature = 45.0;  // 偏差20，超过故障阈值15
    g_pressure = 125.0;    // 偏差25，超过故障阈值20
    sleep(3);  // 等待超过ts时间以触发回调
    print_signal_state("temp_sensor");
    print_signal_state("pressure_sensor");
    
    // 恢复正常
    printf("\n[C Demo] 4. 恢复正常值\n");
    g_temperature = 26.0;
    g_pressure = 102.0;
    sleep(1);
    print_signal_state("temp_sensor");
    print_signal_state("pressure_sensor");
}

int main(void) {
    printf("[C Demo] ToleranceChecker C接口演示程序开始\n");
    printf("[C Demo] ================================\n");
    
    int result;
    
    // 1. 配置温度传感器
    tc_signal_config_t temp_config = {
        .target_value = 25.0,           // 目标温度25度
        .warning_threshold = 8.0,       // 警告阈值：偏差8度
        .fault_threshold = 15.0,        // 故障阈值：偏差15度
        .warning_callback = on_temperature_warning,
        .fault_callback = on_temperature_fault,
        .value_callback = get_temperature_value,
        .tc_ms = 1000,                  // 等待1秒开始监控
        .ts_ms = 2000                   // 持续2秒后触发回调
    };
    
    // 2. 配置压力传感器
    tc_signal_config_t pressure_config = {
        .target_value = 100.0,          // 目标压力100
        .warning_threshold = 12.0,      // 警告阈值：偏差12
        .fault_threshold = 20.0,        // 故障阈值：偏差20
        .warning_callback = on_pressure_warning,
        .fault_callback = on_pressure_fault,
        .value_callback = get_pressure_value,
        .tc_ms = 1000,                  // 等待1秒开始监控
        .ts_ms = 2000                   // 持续2秒后触发回调
    };
    
    // 3. 注册信号
    printf("\n[C Demo] 注册温度传感器...\n");
    result = tc_register_signal("temp_sensor", &temp_config);
    if (result != TC_SUCCESS) {
        printf("[C Demo] 注册温度传感器失败，错误码: %d\n", result);
        return 1;
    }
    
    printf("[C Demo] 注册压力传感器...\n");
    result = tc_register_signal("pressure_sensor", &pressure_config);
    if (result != TC_SUCCESS) {
        printf("[C Demo] 注册压力传感器失败，错误码: %d\n", result);
        return 1;
    }
    
    // 4. 开始监控
    printf("\n[C Demo] 开始监控 (检查间隔: 100ms)...\n");
    result = tc_start_monitoring(100);
    if (result != TC_SUCCESS) {
        printf("[C Demo] 开始监控失败，错误码: %d\n", result);
        return 1;
    }
    
    // 检查监控状态
    if (tc_is_monitoring()) {
        printf("[C Demo] 监控系统已启动\n");
    }
    
    // 5. 等待tc时间过去
    printf("\n[C Demo] 等待tc时间(1秒)，让监控系统初始化...\n");
    sleep(2);
    
    // 6. 模拟数据变化
    simulate_sensor_data_changes();
    
    // 7. 停止监控
    printf("\n[C Demo] 停止监控...\n");
    result = tc_stop_monitoring();
    if (result != TC_SUCCESS) {
        printf("[C Demo] 停止监控失败，错误码: %d\n", result);
    }
    
    // 8. 清理资源
    printf("\n[C Demo] 清理资源...\n");
    tc_remove_signal("temp_sensor");
    tc_remove_signal("pressure_sensor");
    
    printf("\n[C Demo] 演示程序结束\n");
    printf("[C Demo] ================================\n");
    
    return 0;
}