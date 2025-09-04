SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
echo "Building project at: $SCRIPT_DIR"
BUILD_DIR=build/build_java

## Compile java code

mkdir -p build/classes
javac -source 1.7 -target 1.7 \
  -bootclasspath $ANDROID_HOME/platforms/${PLATFORM_VERSION}/android.jar \
  -d $BUILD_DIR \
  $SCRIPT_DIR/com/example/app/*.java

## Convert .class → .dex using D8

mkdir -p build/dex
find $BUILD_DIR -name "*.class" -print0 | xargs -0 $ANDROID_HOME/build-tools/${BUILD_TOOLS_VERSION}/d8 \
  --lib $ANDROID_HOME/platforms/android-34/android.jar \
  --output build/dex

