SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
BUILD_DIR=$(pwd)/build/build_rust
OUTPUT_DIR=$(pwd)/build/native_libs/lib
echo "Building project at: $SCRIPT_DIR"


[[ -d $BUILD_DIR ]] || mkdir -p $BUILD_DIR 

cargo info cargo-ndk --offline >/dev/null 2>&1
if [ $? != 0 ]; then
    cargo install cargo-ndk
fi

cd $SCRIPT_DIR
cargo ndk -t arm64-v8a build --release -o $OUTPUT_DIR --target-dir=$BUILD_DIR --link-libcxx-shared --link-builtins
