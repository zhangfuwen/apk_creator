#pragma once

#include <dlfcn.h>
#include "ndk_utils/log.h"
#include <cstdio>

class Trace {

public:
  static inline void beginSection(const char *format, ...);
  static inline void endSection();
  static inline bool isEnabled() { return is_enabled_; }
  static inline void initialize();

private:
  inline static bool is_enabled_ = false;
  inline static bool has_error_been_shown_ = false;
};



inline static const int TRACE_MAX_SECTION_NAME_LENGTH = 100;

// Tracing functions
inline static void *(*ATrace_beginSection)(const char *sectionName);

inline static void *(*ATrace_endSection)(void);

inline static bool *(*ATrace_isEnabled)(void);

typedef void *(*fp_ATrace_beginSection)(const char *sectionName);

typedef void *(*fp_ATrace_endSection)(void);

typedef bool *(*fp_ATrace_isEnabled)(void);

// inline bool Trace::is_enabled_ = false;
// inline bool Trace::has_error_been_shown_ = false;

inline void Trace::beginSection(const char *fmt, ...) {

  if (is_enabled_) {
    static char buff[TRACE_MAX_SECTION_NAME_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsprintf(buff, fmt, args);
    va_end(args);
    ATrace_beginSection(buff);
  } else if (!has_error_been_shown_) {
    LOGE("Tracing is either not initialized (call Trace::initialize()) "
         "or not supported on this device");
    has_error_been_shown_ = true;
  }
}

inline void Trace::endSection() {

  if (is_enabled_) {
    ATrace_endSection();
  }
}

inline void Trace::initialize() {

  // Using dlsym allows us to use tracing on API 21+ without needing android/trace.h which wasn't
  // published until API 23
  void *lib = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
  if (lib == nullptr) {
    LOGE("Could not open libandroid.so to dynamically load tracing symbols");
  } else {
    ATrace_beginSection =
        reinterpret_cast<fp_ATrace_beginSection >(
            dlsym(lib, "ATrace_beginSection"));
    ATrace_endSection =
        reinterpret_cast<fp_ATrace_endSection >(
            dlsym(lib, "ATrace_endSection"));
    ATrace_isEnabled =
        reinterpret_cast<fp_ATrace_isEnabled >(
            dlsym(lib, "ATrace_isEnabled"));

    if (ATrace_isEnabled != nullptr && ATrace_isEnabled()) {
      is_enabled_ = true;
    }
  }
}
