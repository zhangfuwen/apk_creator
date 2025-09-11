[[ -d build/native_libs ]] || mkdir -p build/native_libs
[[ -d build/native_libs/lib/arm64-v8a ]] || mkdir -p build/native_libs/lib/arm64-v8a
cp build/build_cpp/libmain.so build/native_libs/lib/arm64-v8a/
cp build/build_cpp/ai/libyolov8ncnn.so build/native_libs/lib/arm64-v8a/
