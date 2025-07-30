#include "ToleranceChecker_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep
#include <string.h>

// 传感器数据结构
typedef struct {
    char name[64];              // 传感器名称
    char unit[16];              // 单位
    double value;               // 当前值
    double target_value;        // 目标值
    double warning_threshold;   // 警告阈值
    double fault_threshold;     // 故障阈值
} sensor_data_t;

// 传感器数据实例
static sensor_data_t g_temperature_sensor = {
    .name = "温度传感器",
    .unit = "°C",
    .value = 20.0,
    .target_value = 25.0,
    .warning_threshold = 8.0,
    .fault_threshold = 15.0
};

static sensor_data_t g_pressure_sensor = {
    .name = "压力传感器", 
    .unit = " Pa",
    .value = 100.0,
    .target_value = 100.0,
    .warning_threshold = 12.0,
    .fault_threshold = 20.0
};

// 实现获取传感器值的回调函数
double get_sensor_value(const char* signal_id, void* ctx) {
    sensor_data_t* sensor = (sensor_data_t*)ctx;
    if (!sensor) {
        printf("[C Demo] 错误：传感器上下文为空 - %s\n", signal_id);
        return 0.0;
    }
    
    printf("[C Demo] 获取%s值: %s = %.2f%s\n", sensor->name, signal_id, sensor->value, sensor->unit);
    return sensor->value;
}

// 实现警告回调函数
void on_sensor_warning(const char* signal_id, double value, void* ctx) {
    sensor_data_t* sensor = (sensor_data_t*)ctx;
    if (!sensor) {
        printf("[C Demo] 错误：传感器上下文为空 - %s\n", signal_id);
        return;
    }
    
    printf("[C Demo] ⚠️  %s警告: %s = %.2f%s\n", sensor->name, signal_id, value, sensor->unit);
}

void on_sensor_fault(const char* signal_id, double value, void* ctx) {
    sensor_data_t* sensor = (sensor_data_t*)ctx;
    if (!sensor) {
        printf("[C Demo] 错误：传感器上下文为空 - %s\n", signal_id);
        return;
    }
    
    printf("[C Demo] ❌ %s故障: %s = %.2f%s\n", sensor->name, signal_id, value, sensor->unit);
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
    g_temperature_sensor.value = 25.0;  // 目标值25，在容差范围内
    g_pressure_sensor.value = 105.0;    // 目标值100，在容差范围内
    sleep(1);
    print_signal_state("temp_sensor");
    print_signal_state("pressure_sensor");
    
    // 警告状态
    printf("\n[C Demo] 2. 设置警告级别的值\n");
    g_temperature_sensor.value = 35.0;  // 偏差10，超过警告阈值8
    g_pressure_sensor.value = 115.0;    // 偏差15，超过警告阈值12
    sleep(3);  // 等待超过ts时间(2秒)以触发回调
    print_signal_state("temp_sensor");
    print_signal_state("pressure_sensor");
    
    // 故障状态
    printf("\n[C Demo] 3. 设置故障级别的值\n");
    g_temperature_sensor.value = 45.0;  // 偏差20，超过故障阈值15
    g_pressure_sensor.value = 125.0;    // 偏差25，超过故障阈值20
    sleep(3);  // 等待超过ts时间以触发回调
    print_signal_state("temp_sensor");
    print_signal_state("pressure_sensor");
    
    // 恢复正常
    printf("\n[C Demo] 4. 恢复正常值\n");
    g_temperature_sensor.value = 26.0;
    g_pressure_sensor.value = 102.0;
    sleep(1);
    print_signal_state("temp_sensor");
    print_signal_state("pressure_sensor");
}

int main(void) {
    printf("[C Demo] ToleranceChecker C接口演示程序开始\n");
    printf("[C Demo] ================================\n");
    
    int result;
    
    // 1. 配置温度传感器
    tc_signal_config_t temp_config;
    temp_config.target_value = g_temperature_sensor.target_value;
    temp_config.warning_threshold = g_temperature_sensor.warning_threshold;
    temp_config.fault_threshold = g_temperature_sensor.fault_threshold;
    temp_config.warning_callback = on_sensor_warning;
    temp_config.fault_callback = on_sensor_fault;
    temp_config.value_callback = get_sensor_value;
    temp_config.context = &g_temperature_sensor;  // 传递传感器上下文
    temp_config.tc_ms = 1000;                     // 等待1秒开始监控
    temp_config.ts_ms = 2000;                     // 持续2秒后触发回调
    
    // 2. 配置压力传感器
    tc_signal_config_t pressure_config;
    pressure_config.target_value = g_pressure_sensor.target_value;
    pressure_config.warning_threshold = g_pressure_sensor.warning_threshold;
    pressure_config.fault_threshold = g_pressure_sensor.fault_threshold;
    pressure_config.warning_callback = on_sensor_warning;
    pressure_config.fault_callback = on_sensor_fault;
    pressure_config.value_callback = get_sensor_value;
    pressure_config.context = &g_pressure_sensor;  // 传递传感器上下文
    pressure_config.tc_ms = 1000;                  // 等待1秒开始监控
    pressure_config.ts_ms = 2000;                  // 持续2秒后触发回调
    
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
    printf("\n[C Demo] 上下文优化亮点:\n");
    printf("[C Demo] 1. 每个传感器有独立的数据结构\n");
    printf("[C Demo] 2. 回调函数通过上下文访问传感器信息\n");
    printf("[C Demo] 3. 支持多实例，无需全局变量\n");
    printf("[C Demo] 4. 更好的封装和可维护性\n");
    
    return 0;
}