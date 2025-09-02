
# Android Apk构建与拆解

## 总体流程

总体流程涉及五个工具，也即5个步骤：

```bash
javac – compile Java code
d8 – convert .class to .dex (Google's DEX compiler)
aapt2 – compile and link resources
zipalign – align the APK
apksigner – sign the APK
```


## 工程准备

```
MyApp/
├── src/
│   └── com/example/app/MainActivity.java
├── res/
│   └── values/
│       └── strings.xml
│   └── layout/
│       └── activity_main.xml
├── AndroidManifest.xml
└── build/
```


## Download build tools

Set environment variables:

```bash
export NDK_VERSION="27.3.13750724"
export PLATFORM_VERSION="android-34"
export CMAKE_VERSION="3.10.2.4988404"
export BUILD_TOOLS_VERSION="34.0.0"

export ANDROID_HOME=$HOME/bin/android-sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/$NDK_VERSION
export ANDROID_NDK_ROOT=$ANDROID_NDK_HOME
export ANDROID_SDK=$ANDROID_HOME

echo "ANDROID_HOME=$ANDROID_HOME"

```

Download build tools:

```bash

export URL_CLITOOLS=https://dl.google.com/android/repository/commandlinetools-linux-6200805_latest.zip
export TMPFILE="$HOME/temp.zip"
[[ -d $ANDROID_HOME ]] || mkdir -p $ANDROID_HOME && wget --quiet ${URL_CLITOOLS} -O ${TMPFILE} \
    && unzip -d ${ANDROID_HOME} ${TMPFILE} \
    && rm ${TMPFILE}
```

Install SDK, NDK and other tools:

```bash

yes | $ANDROID_HOME/tools/bin/sdkmanager --sdk_root=${ANDROID_HOME} --licenses \
    && $ANDROID_HOME/tools/bin/sdkmanager --sdk_root=${ANDROID_HOME} "build-tools;${BUILD_TOOLS_VERSION}" "cmake;${CMAKE_VERSION}" "ndk;${NDK_VERSION}" "platform-tools" "platforms;${PLATFORM_VERSION}" "tools"
```

## Compile java code

```bash
mkdir -p build/classes
javac -source 1.7 -target 1.7 \
  -bootclasspath $ANDROID_HOME/platforms/${PLATFORM_VERSION}/android.jar \
  -d build/classes \
  src/com/example/app/*.java

```

## Convert .class → .dex using D8

```bash
mkdir -p build/dex
find build/classes -name "*.class" -print0 | xargs -0 $ANDROID_HOME/build-tools/${BUILD_TOOLS_VERSION}/d8 \
  --lib $ANDROID_HOME/platforms/android-34/android.jar \
  --output build/dex
```

## Using aapt2 to compile and link resources

### Compile Resources with aapt2

```bash
mkdir -p build/res-compiled

$ANDROID_HOME/build-tools/${BUILD_TOOLS_VERSION}/aapt2 compile \
  --dir res \
  -o build/res-compiled/

```

### Link resources into resources.arsc and final APK

```bash
mkdir -p build/apk

$ANDROID_HOME/build-tools/${BUILD_TOOLS_VERSION}/aapt2 link \
  --manifest AndroidManifest.xml \
  -I $ANDROID_HOME/platforms/${PLATFORM_VERSION}/android.jar \
  --auto-add-overlay \
  --java build/gen \
  --no-version-vectors \
  -o build/apk/app-unaligned.apk \
  --resources-dir build/res-compiled \
  --allow-reserved-package-id \
  --package-id 0x7f

```

## Add DEX file to APK 

```bash

zip -g build/apk/app-unaligned.apk build/dex/classes.dex

```

## Align the APK with zipalign

```bash

$ANDROID_HOME/build-tools/34.0.0/zipalign \
  -f 4 \
  build/apk/app-unaligned.apk \
  build/apk/app-aligned.apk

```

## Sign the APK with apksigner

### Generate a key if you don't have one

```bash
keytool -genkeypair \
  -alias mykey \
  -keyalg RSA \
  -keysize 2048 \
  -validity 10000 \
  -keystore mykey.jks \
  -storepass android \
  -keypass android \
  -dname "CN=,OU=,O=,L=,S=,C=US"
```

### Sign the APK

```bash
$ANDROID_HOME/build-tools/34.0.0/apksigner sign \
  --key-pass pass:android \
  --ks-pass pass:android \
  --ks mykey.jks \
  --out build/apk/app-signed.apk \
  build/apk/app-aligned.apk
```

## Install the APK

```bash
adb install build/apk/app-signed.apk
```

