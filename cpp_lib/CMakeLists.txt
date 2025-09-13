# you have to have CMake >= 3.20 to be able to use CMakePresets.json
cmake_minimum_required(VERSION 3.20)
project("MyNativeApp" LANGUAGES C CXX)

message("CMAKE_VERSION: ${CMAKE_VERSION}")
message("CMAKE_ANDROID_NDK: ${CMAKE_ANDROID_NDK}")
message("ANDROID_PLATFORM: ${ANDROID_PLATFORM}")

include_directories(${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${CMAKE_ANDROID_NDK}/sources/cxx-stl/llvm-libc++/include)
include_directories(
  ${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include
)

include(FetchContent)

set(NCNN_BUILD_EXAMPLES
    OFF
    CACHE BOOL "Build ncnn examples" FORCE
)
set(NCNN_BUILD_TOOLS
    OFF
    CACHE BOOL "Build ncnn tools" FORCE
)
set(NCNN_VULKAN
    ON
    CACHE BOOL "Enable Vulkan support" FORCE
)

FetchContent_Declare(
  ncnn
  URL http://127.0.0.1:8000/ncnn-20250503-android-vulkan-shared.zip
      https://github.com/Tencent/ncnn/releases/download/20250503/ncnn-20250503-android-vulkan-shared.zip
  URL_HASH MD5=ad8e640e68a71f83dd8979bc78ce6766
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/ncnn/src DOWNLOAD_EXTRACT_TIMESTAMP
  true
)

# FetchContent_Declare( ncnn GIT_REPOSITORY https://github.com/Tencent/ncnn.git
# GIT_TAG 20250503 SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/ncnn/src )

FetchContent_MakeAvailable(ncnn)

# Find required libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)
find_library(OpenSLES-lib OpenSLES)
find_library(camera-lib camera2ndk)
find_library(media-lib mediandk)

# Build native_app_glue (required for NativeActivity) build native_app_glue as a
# static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(
  native_app_glue STATIC
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)
# set_target_properties(native_app_glue PROPERTIES LINKER_LANGUAGE CXX)
# set_target_properties(native_app_glue PROPERTIES COMPILE_FLAGS
# "-Wno-error=unused-parameter")

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate"
)

set(OBOE_ENABLE_OPENSL
    OFF
    CACHE BOOL "Enable OpenSL ES backend"
)

set(OBOE_INCLUDE_DIR ${VCPKG_ROOT}/installed/arm64-android/include)
set(OBOE_LINK_DIR ${VCPKG_ROOT}/installed/arm64-android/lib)

set(NCNN_INCLUDE_DIR ${PROJECT_BINARY_DIR}/ncnn/src/arm64-v8a/include)
set(NCNN_LINK_DIR ${PROJECT_BINARY_DIR}/ncnn/src/arm64-v8a/lib)

add_subdirectory(ai)
target_include_directories(yolov8ncnn PUBLIC ${NCNN_INCLUDE_DIR})
target_link_directories(yolov8ncnn PUBLIC ${NCNN_LINK_DIR})
target_link_libraries(yolov8ncnn PUBLIC ${log-lib} ${android-lib} ${camera-lib} ${media-lib})

# Main native library
add_library(
  main SHARED
  audio/LatencyTuningCallback.cpp
  audio/OboeEngine.cpp
  audio/SoundGenerator.cpp
  camera/camera_engine.cpp
  camera/camera_listeners.cpp
  camera/camera_manager.cpp
  camera/camera_utils.cpp
  camera/image_reader.cpp
  main.cpp
  renderer/eye_renderer.cpp
  renderer/rectangles_renderer.cpp
)
set_target_properties(
  main
  PROPERTIES LINKER_LANGUAGE CXX
             CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF
)
target_include_directories(
  main SYSTEM
  PRIVATE ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
          ${OBOE_INCLUDE_DIR} ${CMAKE_SOURCE_DIR} ${NCNN_INCLUDE_DIR}
)
target_link_directories(main PRIVATE ${OBOE_LINK_DIR} ${NCNN_LINK_DIR})
target_compile_options(
  main PRIVATE -Wall -Wextra -Werror # Treat all warnings as errors
               -DRENDER_CAM_TO_WINDOW=0
               -DNCNN_VULKAN=1
)

# Extract numeric API level from ANDROID_PLATFORM (e.g., android-24 → 24)
if(ANDROID AND ANDROID_PLATFORM)
  string(REGEX MATCH "[0-9]+" ANDROID_API_NUM "${ANDROID_PLATFORM}")
  message(STATUS "Target Android API Level: ${ANDROID_API_NUM}")

  # Define __ANDROID_API__ manually (usually already set)
  # target_compile_definitions(main PRIVATE __ANDROID_API__=${ANDROID_API_NUM})

endif()

# Link libraries
target_link_libraries(
  main
  android
  oboe
  native_app_glue
  yolov8ncnn
  ${log-lib}
  ${android-lib}
  ${EGL-lib}
  ${GLESv3-lib}
  ${OpenSLES-lib}
  ${camera-lib}
  ${media-lib}
)
