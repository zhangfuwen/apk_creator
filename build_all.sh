download_tools=0
if [[ $# -eq 1 ]] && [[  "$1" == "-d" ]]; then
    download_tools=1
fi

if [[ $download_tools == "1" ]]; then
    bash scripts/android_set.sh
fi

source .ndk_env

source scripts/vcpkg_setup.sh
source scripts/rust_setup.sh

echo "## install oboe"
vcpkg install oboe:arm64-android
vcpkg install opencv4:arm64-android

bash java/build.sh \
&& bash cpp_lib/build.sh \
&& bash cpp_lib/copy.sh \
&& bash rust_lib/build.sh \
&& bash rust_lib/copy.sh \
&& bash scripts/build_pack.sh
