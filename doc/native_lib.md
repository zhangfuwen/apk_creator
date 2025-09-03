# Compilation of native library

## Build native library

```bash
# set src dir and build dir and run cmake
[[ -d build/native_lib ]] || mkdir build/native_lib

# if you don't specify a preset, it won't use CMakePresets.json
PATH=$PATH:${ANDROID_SDK}/cmake/${CMAKE_VERSION}/bin \
${ANDROID_SDK}/cmake/${CMAKE_VERSION}/bin/cmake -S lib -B build/native_lib --preset=android-arm64

PATH=$PATH:${ANDROID_SDK}/cmake/${CMAKE_VERSION}/bin VERBOSE=1 \
${ANDROID_SDK}/cmake/${CMAKE_VERSION}/bin/cmake --build build/native_lib

``````
