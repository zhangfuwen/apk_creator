#pragma once
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "NativeOpenGL", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NativeOpenGL", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "NativeOpenGL", __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "NativeOpenGL", __VA_ARGS__)
#define LOG_LINE() __android_log_print(ANDROID_LOG_INFO, "NativeOpenGL", "%s:%d", __FILE__, __LINE__)
