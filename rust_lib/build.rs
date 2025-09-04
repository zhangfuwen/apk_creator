// build.rs
fn main() {
    // Link libpthread (required for std::thread, etc.)
//    println!("cargo:rustc-link-lib=dylib=pthread");

    // Optional: also link C++ runtime if using C++ (e.g., OpenCV)
 //   println!("cargo:rustc-link-lib=dylib=c++");
    if std::env::var("TARGET").unwrap().contains("android") {
        println!("cargo:rustc-link-lib=c++_static");
        println!("cargo:warning=Linking statically against libc++ (c++_static)");
    } else {
        // Optional: link dynamically on other platforms (or omit)
        println!("cargo:rustc-link-lib=c++");
    }
    let vcpkg_dir = std::env::var("VCPKG_ROOT").unwrap();
    let target_arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();
    let abi = match target_arch.as_str() {
        "aarch64" => "arm64-v8a",
        "arm" => "armeabi-v7a",
        _ => panic!("Unsupported arch"),
    };

    let opencv_lib_dir = format!("{}/installed/arm64-android/lib", vcpkg_dir);
    let opencv_lib_dir2 = format!("{}/installed/arm64-android/sdk/native/staticlibs/arm64-v8a", vcpkg_dir);
    let opencv_lib_dir3 = format!("{}/installed/arm64-android/lib/manual-link/opencv4_thirdparty", vcpkg_dir);
    let ndk_lib_dir = format!("{}/toolchains/llvm/prebuilt/linux-x86_64/lib64/clang/*/lib/android/arm64-v8a", std::env::var("ANDROID_NDK_ROOT").unwrap());

    println!("cargo:rustc-link-search=native={}", opencv_lib_dir);
    println!("cargo:rustc-link-search=native={}", opencv_lib_dir2);
    println!("cargo:rustc-link-search=native={}", opencv_lib_dir3);
    println!("cargo:rustc-link-lib=static=opencv_core4");
    println!("cargo:rustc-link-lib=static=opencv_imgproc4");
    println!("cargo:rustc-link-lib=static=opencv_imgcodecs4");
    println!("cargo:rustc-link-lib=static=kleidicv");
    println!("cargo:rustc-link-lib=static=kleidicv_thread");
    println!("cargo:rustc-link-lib=static=kleidicv_hal");

}
