# you have to have CMake >= 3.20 to be able to use CMakePresets.json
cmake_minimum_required(VERSION 3.20)
project("MyNativeApp" LANGUAGES C CXX)

message("CMAKE_VERSION: ${CMAKE_VERSION}")
message("CMAKE_ANDROID_NDK: ${CMAKE_ANDROID_NDK}")

include_directories(${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${CMAKE_ANDROID_NDK}/sources/cxx-stl/llvm-libc++/include)
include_directories(
  ${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include
)

# Find required libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

# Build native_app_glue (required for NativeActivity) build native_app_glue as a
# static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(
  native_app_glue STATIC
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
# set_target_properties(native_app_glue PROPERTIES LINKER_LANGUAGE CXX)
# set_target_properties(native_app_glue PROPERTIES COMPILE_FLAGS
# "-Wno-error=unused-parameter")

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# Main native library
add_library(main SHARED main.cpp
    rectangles_renderer.cpp
    eye_renderer.cpp
)
set_target_properties(main PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(
  main PRIVATE ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)

# Link libraries
target_link_libraries(
  main
  android
  native_app_glue
  ${log-lib}
  ${android-lib}
  ${EGL-lib}
  ${GLESv3-lib})
