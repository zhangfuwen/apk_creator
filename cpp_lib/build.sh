[[ -d build/build_cpp ]] || mkdir build/build_cpp

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
echo "Building project at: $SCRIPT_DIR"
# if you don't specify a preset, it won't use CMakePresets.json
PATH=$PATH:${ANDROID_SDK}/cmake/${CMAKE_VERSION}/bin \
${ANDROID_SDK}/cmake/${CMAKE_VERSION}/bin/cmake  -Wno-deprecated  -S $SCRIPT_DIR -B build/build_cpp --preset=android-arm64 \
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

PATH=$PATH:${ANDROID_SDK}/cmake/${CMAKE_VERSION}/bin  VERBOSE=1 \
${ANDROID_SDK}/cmake/${CMAKE_VERSION}/bin/cmake  --build build/build_cpp
