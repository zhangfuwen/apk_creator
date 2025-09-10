download_tools=0
if [[ $# -eq 1 ]] && [[  "$1" == "-d" ]]; then
    download_tools=1
fi

echo "## Set environment variables:"

export NDK_VERSION="27.3.13750724"
export PLATFORM_VERSION="android-34"
export CMAKE_VERSION="3.10.2.4988404"
export BUILD_TOOLS_VERSION="34.0.0"

export ANDROID_HOME=$HOME/bin/android-sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/$NDK_VERSION
export ANDROID_NDK_ROOT=$ANDROID_NDK_HOME
export ANDROID_SDK=$ANDROID_HOME

echo "ANDROID_HOME=$ANDROID_HOME"


echo "## Download build tools:"

if [[ "$download_tools" == "1" ]]; then
    echo "Downloading build tools: $download_tools"
    export URL_CLITOOLS=https://dl.google.com/android/repository/commandlinetools-linux-6200805_latest.zip
    export TMPFILE="$HOME/temp.zip"
    [[ -d $ANDROID_HOME ]] || mkdir -p $ANDROID_HOME && wget --quiet ${URL_CLITOOLS} -O ${TMPFILE} \
        && unzip -d ${ANDROID_HOME} ${TMPFILE} \
        && rm ${TMPFILE}

    yes | $ANDROID_HOME/tools/bin/sdkmanager --sdk_root=${ANDROID_HOME} --licenses \
        && $ANDROID_HOME/tools/bin/sdkmanager --sdk_root=${ANDROID_HOME} "build-tools;${BUILD_TOOLS_VERSION}" "cmake;${CMAKE_VERSION}" "ndk;${NDK_VERSION}" "platform-tools" "platforms;${PLATFORM_VERSION}" "tools"

fi

echo "# Compile java code"

mkdir -p build/classes
javac -source 1.7 -target 1.7 \
  -bootclasspath $ANDROID_HOME/platforms/${PLATFORM_VERSION}/android.jar \
  -d build/classes \
  src/com/example/app/*.java


echo "## Convert .class → .dex using D8"

mkdir -p build/dex
find build/classes -name "*.class" -print0 | xargs -0 $ANDROID_HOME/build-tools/${BUILD_TOOLS_VERSION}/d8 \
  --lib $ANDROID_HOME/platforms/android-34/android.jar \
  --output build/dex

echo "## Compile Resources with aapt2"

mkdir -p build/res-compiled

$ANDROID_HOME/build-tools/${BUILD_TOOLS_VERSION}/aapt2 compile \
  --dir res \
  -o build/res-compiled/

echo "## Link resources into resources.arsc and final APK"

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

echo "## Add DEX file to APK "

cd build/dex
zip -g ../apk/app-unaligned.apk classes.dex
cd -

echo "## Align the APK with zipalign"


$ANDROID_HOME/build-tools/34.0.0/zipalign \
  -f 4 \
  build/apk/app-unaligned.apk \
  build/apk/app-aligned.apk

echo "## Generate a key if you don't have one"

if [[ ! -f mykey.jks ]]; then

keytool -genkeypair \
  -alias mykey \
  -keyalg RSA \
  -keysize 2048 \
  -validity 10000 \
  -keystore mykey.jks \
  -storepass android \
  -keypass android \
  -dname "CN=,OU=,O=,L=,S=,C=US"
fi

echo "## Sign the APK"

$ANDROID_HOME/build-tools/34.0.0/apksigner sign \
  --key-pass pass:android \
  --ks-pass pass:android \
  --ks mykey.jks \
  --out build/apk/app-signed.apk \
  build/apk/app-aligned.apk

unzip -l build/apk/app-signed.apk
