#pragma once
#include <android/log.h>
#ifdef __cplusplus
#    include <string_view>
#    include <array>
#    include <algorithm>

constexpr int TRUNC_SIZE = 10;

// Compile-time filename extraction
constexpr std::string_view get_filename(const std::string_view path) {
    if (path.empty())
        return "";

    // Find last occurrence of directory separators
    const size_t last_slash = path.find_last_of("/\\");

    if (last_slash == std::string_view::npos) {
        return path;
    }

    return path.substr(last_slash + 1);
}

#    define PRINT(LEVEL, fmt, ...)                                                                                     \
        __android_log_print(LEVEL, "NativeOpenGL", "(%10.10s:%4.4d) %10.10s: " fmt, get_filename(__FILE__).data(),     \
                            __LINE__, __func__, ##__VA_ARGS__)
#else
#    define PRINT(LEVEL, fmt, ...)                                                                                     \
        __android_log_print(LEVEL, "NativeOpenGL", "(%s:%d %s): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif

#define LOGI(fmt, ...) PRINT(ANDROID_LOG_INFO, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) PRINT(ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) PRINT(ANDROID_LOG_WARN, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) PRINT(ANDROID_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOGV(fmt, ...) PRINT(ANDROID_LOG_VERBOSE, fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) PRINT(ANDROID_LOG_FATAL, fmt, ##__VA_ARGS__)
#define LOGS(fmt, ...) PRINT(ANDROID_LOG_SILENT, fmt, ##__VA_ARGS__)

#define LOG_LINE() LOGI("line_trace") 

#define ASSERT(x, ...) if (!(x)) { LOGF(__VA_ARGS__); abort(); } 
