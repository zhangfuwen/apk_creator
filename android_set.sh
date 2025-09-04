
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

export URL_CLITOOLS=https://dl.google.com/android/repository/commandlinetools-mac-6200805_latest.zip
export TMPFILE="$HOME/temp.zip"
[[ -d $ANDROID_HOME ]] || mkdir -p $ANDROID_HOME

    wget --quiet ${URL_CLITOOLS} -O ${TMPFILE} \
    && unzip -d ${ANDROID_HOME} ${TMPFILE} \
    && rm ${TMPFILE}


#Install SDK, NDK and other tools:

yes | $ANDROID_HOME/tools/bin/sdkmanager --sdk_root=${ANDROID_HOME} --licenses \
    && $ANDROID_HOME/tools/bin/sdkmanager --sdk_root=${ANDROID_HOME} "build-tools;${BUILD_TOOLS_VERSION}" "cmake;${CMAKE_VERSION}" "ndk;${NDK_VERSION}" "platform-tools" "platforms;${PLATFORM_VERSION}" "tools"
