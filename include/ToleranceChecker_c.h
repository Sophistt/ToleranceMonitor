#ifndef TOLERANCE_CHECKER_C_H
#define TOLERANCE_CHECKER_C_H

#ifdef __cplusplus
extern "C" {
#endif

// 信号状态枚举
typedef enum {
    TC_SIGNAL_UNKNOWN = 0,  // 初始未知状态
    TC_SIGNAL_NORMAL,       // 正常状态
    TC_SIGNAL_WARNING,      // 警告状态
    TC_SIGNAL_FAULT         // 故障状态
} tc_signal_state_t;

// 回调函数类型定义
typedef void (*tc_warning_callback_t)(const char* signal_id, double value, void* ctx);
typedef void (*tc_fault_callback_t)(const char* signal_id, double value, void* ctx);
typedef double (*tc_value_callback_t)(const char* signal_id, void* ctx);

// 信号配置结构
typedef struct {
    double target_value;                // 目标值
    double warning_threshold;           // 容差警告阈值（偏差的绝对值）
    double fault_threshold;             // 容差故障阈值（偏差的绝对值）
    tc_warning_callback_t warning_callback;  // 警告回调函数
    tc_fault_callback_t fault_callback;      // 故障回调函数
    tc_value_callback_t value_callback;      // 获取信号值的回调函数
    void* context;                      // 用户上下文指针（调用者负责生命周期管理）
    int tc_ms;                          // 等待时间（毫秒）
    int ts_ms;                          // 持续时间（毫秒）
} tc_signal_config_t;

/**
 * 重要说明：context 生命周期管理
 * 
 * context 指针由调用者负责管理，调用者必须确保：
 * 1. context 指向的内存在信号监控期间保持有效
 * 2. 在调用 tc_remove_signal() 或程序结束前，不能释放 context 指向的内存
 * 3. 这是标准的 C 接口设计模式，遵循"谁分配谁释放"的原则
 */

// 错误码定义
#define TC_SUCCESS           0    // 成功
#define TC_ERROR_GENERAL    -1    // 一般错误
#define TC_ERROR_EXISTS     -2    // 信号已存在
#define TC_ERROR_NOT_FOUND  -3    // 信号未找到
#define TC_ERROR_MONITORING -4    // 监控状态错误
#define TC_ERROR_NULL_PTR   -5    // 空指针错误

// API 函数声明

/**
 * 注册信号
 * @param signal_id 信号ID字符串
 * @param config 信号配置结构指针
 * @return 成功返回TC_SUCCESS，失败返回错误码
 */
int tc_register_signal(const char* signal_id, const tc_signal_config_t* config);

/**
 * 停止监控
 * @return 成功返回TC_SUCCESS，失败返回错误码
 */
int tc_stop_monitoring(void);

/**
 * 检查是否正在监控
 * @return 1表示正在监控，0表示未监控
 */
int tc_is_monitoring(void);

/**
 * 移除信号
 * @param signal_id 信号ID字符串
 * @return 成功返回TC_SUCCESS，失败返回错误码
 */
int tc_remove_signal(const char* signal_id);

/**
 * 获取信号状态
 * @param signal_id 信号ID字符串
 * @param state 输出参数，存储信号状态
 * @return 成功返回TC_SUCCESS，失败返回错误码
 */
int tc_get_signal_state(const char* signal_id, tc_signal_state_t* state);

/**
 * 获取状态名称字符串（用于调试）
 * @param state 信号状态
 * @return 状态名称字符串
 */
const char* tc_get_state_name(tc_signal_state_t state);

#ifdef __cplusplus
}
#endif

#endif // TOLERANCE_CHECKER_C_H