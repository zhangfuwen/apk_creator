#pragma once
#include <time.h>
#include <stdint.h>

#define STRINGIFY(x)      #x
#define TOSTRING(x)       STRINGIFY(x)
#define PRINT_MACRO(name) _Pragma(TOSTRING(message(#name " = " TOSTRING(name))))

PRINT_MACRO(__ANDROID_API__)
inline uint64_t get_time_millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
inline double get_time_second() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1000000000;
}
