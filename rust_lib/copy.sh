[[ -d build/native_libs ]] || mkdir -p build/native_libs
[[ -d build/native_libs/lib/arm64-v8a ]] || mkdir -p build/native_libs/lib/arm64-v8a

cp build/build_rust/aarch64-linux-android/release/librust_lib.so build/native_libs/lib/arm64-v8a/
