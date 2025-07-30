#include "ToleranceChecker_c.h"
#include "ToleranceChecker.h"
#include <string>
#include <exception>

// 将 C 回调函数转换为 C++ std::function
static WarningCallback wrap_warning_callback(tc_warning_callback_t c_callback, void* context) {
    if (!c_callback) return nullptr;
    return [c_callback, context](const std::string& signalId, double value) {
        c_callback(signalId.c_str(), value, context);
    };
}

static FaultCallback wrap_fault_callback(tc_fault_callback_t c_callback, void* context) {
    if (!c_callback) return nullptr;
    return [c_callback, context](const std::string& signalId, double value) {
        c_callback(signalId.c_str(), value, context);
    };
}

static ValueCallback wrap_value_callback(tc_value_callback_t c_callback, void* context) {
    if (!c_callback) return nullptr;
    return [c_callback, context](const std::string& signalId) -> double {
        return c_callback(signalId.c_str(), context);
    };
}

// 将 C 信号状态转换为 C++ 信号状态
static SignalState convert_to_cpp_state(tc_signal_state_t c_state) {
    switch (c_state) {
        case TC_SIGNAL_UNKNOWN: return SignalState::UNKNOWN;
        case TC_SIGNAL_NORMAL: return SignalState::NORMAL;
        case TC_SIGNAL_WARNING: return SignalState::WARNING;
        case TC_SIGNAL_FAULT: return SignalState::FAULT;
        default: return SignalState::UNKNOWN;
    }
}

// 将 C++ 信号状态转换为 C 信号状态
static tc_signal_state_t convert_to_c_state(SignalState cpp_state) {
    switch (cpp_state) {
        case SignalState::UNKNOWN: return TC_SIGNAL_UNKNOWN;
        case SignalState::NORMAL: return TC_SIGNAL_NORMAL;
        case SignalState::WARNING: return TC_SIGNAL_WARNING;
        case SignalState::FAULT: return TC_SIGNAL_FAULT;
        default: return TC_SIGNAL_UNKNOWN;
    }
}

// API 函数实现

int tc_register_signal(const char* signal_id, const tc_signal_config_t* config) {
    if (!signal_id || !config) {
        return TC_ERROR_NULL_PTR;
    }
    
    try {
        // 创建 C++ 配置
        SignalConfig cpp_config;
        cpp_config.targetValue = config->target_value;
        cpp_config.warningThreshold = config->warning_threshold;
        cpp_config.faultThreshold = config->fault_threshold;
        cpp_config.warningCallback = wrap_warning_callback(config->warning_callback, config->context);
        cpp_config.faultCallback = wrap_fault_callback(config->fault_callback, config->context);
        cpp_config.valueCallback = wrap_value_callback(config->value_callback, config->context);
        cpp_config.tcMs = config->tc_ms;
        cpp_config.tsMs = config->ts_ms;
        
        // 注册信号
        std::string signal_key(signal_id);
        auto& checker = ToleranceChecker::getInstance();
        bool success = checker.registerSignal(signal_key, cpp_config);
        
        return success ? TC_SUCCESS : TC_ERROR_EXISTS;
        
    } catch (const std::exception& e) {
        return TC_ERROR_GENERAL;
    }
}

int tc_stop_monitoring(void) {
    try {
        auto& checker = ToleranceChecker::getInstance();
        checker.stopMonitoring();
        return TC_SUCCESS;
        
    } catch (const std::exception& e) {
        return TC_ERROR_GENERAL;
    }
}

int tc_is_monitoring(void) {
    try {
        auto& checker = ToleranceChecker::getInstance();
        return checker.isMonitoring() ? 1 : 0;
        
    } catch (const std::exception& e) {
        return 0;
    }
}

int tc_remove_signal(const char* signal_id) {
    if (!signal_id) {
        return TC_ERROR_NULL_PTR;
    }
    
    try {
        std::string signal_key(signal_id);
        auto& checker = ToleranceChecker::getInstance();
        checker.removeSignal(signal_key);
        
        return TC_SUCCESS;
        
    } catch (const std::exception& e) {
        return TC_ERROR_GENERAL;
    }
}

int tc_get_signal_state(const char* signal_id, tc_signal_state_t* state) {
    if (!signal_id || !state) {
        return TC_ERROR_NULL_PTR;
    }
    
    try {
        std::string signal_key(signal_id);
        auto& checker = ToleranceChecker::getInstance();
        SignalState cpp_state = checker.getSignalState(signal_key);
        
        *state = convert_to_c_state(cpp_state);
        return TC_SUCCESS;
        
    } catch (const std::exception& e) {
        return TC_ERROR_GENERAL;
    }
}

const char* tc_get_state_name(tc_signal_state_t state) {
    switch (state) {
        case TC_SIGNAL_UNKNOWN: return "UNKNOWN";
        case TC_SIGNAL_NORMAL: return "NORMAL";
        case TC_SIGNAL_WARNING: return "WARNING";
        case TC_SIGNAL_FAULT: return "FAULT";
        default: return "INVALID";
    }
}