
# Set environment variables:

# You have to have this version of JDK, otherwise it won't work
# To install this version of JDK, you can use `apt install openjdk-8-jdk`
JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 

export NDK_VERSION="27.3.13750724"
export PLATFORM_VERSION_NUMBER="34"
export PLATFORM_VERSION="android-${PLATFORM_VERSION_NUMBER}"
export CMAKE_VERSION="4.1.0"
export BUILD_TOOLS_VERSION="34.0.0"

export ANDROID_HOME=$HOME/bin/android-sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/$NDK_VERSION
export ANDROID_NDK_ROOT=$ANDROID_NDK_HOME
export ANDROID_SDK=$ANDROID_HOME

echo "ANDROID_HOME=$ANDROID_HOME"

echo "Compile Resources with aapt2"
mkdir -p build/res-compiled

$ANDROID_HOME/build-tools/${BUILD_TOOLS_VERSION}/aapt2 compile \
  --dir res \
  -o build/res-compiled/

echo "Link resources into resources.arsc and final APK"
mkdir -p build/apk
$ANDROID_HOME/build-tools/${BUILD_TOOLS_VERSION}/aapt2 link \
  --manifest AndroidManifest.xml \
  -I $ANDROID_HOME/platforms/${PLATFORM_VERSION}/android.jar \
  --auto-add-overlay \
  --java build/gen \
  --no-version-vectors \
  -o build/apk/app-unaligned.apk \
  --allow-reserved-package-id \
  --package-id 0x7f \
   build/res-compiled/*.flat

echo "Add assets to apk"
find assets -type f -exec zip build/apk/app-unaligned.apk {} \;


echo "Add DEX file to APK"
if [[ -d build/dex ]]; then
    cd build/dex
    zip -g ../apk/app-unaligned.apk classes.dex
    cd ../..
fi

echo "Add library to apk"
if [[ -d build/native_libs ]]; then
    cd build/native_libs
    zip -g ../apk/app-unaligned.apk lib/arm64-v8a/libmain.so
    zip -g ../apk/app-unaligned.apk lib/arm64-v8a/libyolov8ncnn.so
    zip -g ../apk/app-unaligned.apk lib/arm64-v8a/libncnn.so
    cd ../..
fi

echo "Add rust library to apk"
if [[ -f build/native_libs/lib/arm64-v8a/librust_lib.so ]]; then
    cd build/native_libs
    zip -g ../apk/app-unaligned.apk lib/arm64-v8a/librust_lib.so
    cd ../..
fi
if [[ -f build/native_libs/lib/arm64-v8a/libc++_shared.so ]]; then
    cd build/native_libs
    zip -g ../apk/app-unaligned.apk lib/arm64-v8a/libc++_shared.so
    cd ../..
fi


echo "Align the APK with zipalign"
$ANDROID_HOME/build-tools/34.0.0/zipalign \
  -f 4 \
  build/apk/app-unaligned.apk \
  build/apk/app-aligned.apk

if [[ ! -e mykey.jks ]]; then
    echo "Generating mykey.jks"
    keytool -genkeypair \
      -alias mykey \
      -keyalg RSA \
      -keysize 2048 \
      -validity 10000 \
      -keystore mykey.jks \
      -storepass android \
      -keypass android \
      -dname "CN=,OU=,O=,L=,S=,C=US"
else
    echo "Using local mykey.jks"
fi

echo "Sign the APK"
$ANDROID_HOME/build-tools/34.0.0/apksigner sign \
  --key-pass pass:android \
  --ks-pass pass:android \
  --ks mykey.jks \
  --out build/apk/app-signed.apk \
  build/apk/app-aligned.apk

echo "Show APK content:"
unzip -l build/apk/app-signed.apk

