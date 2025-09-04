// build.rs
fn main() {
    // Link libpthread (required for std::thread, etc.)
//    println!("cargo:rustc-link-lib=dylib=pthread");

    // Optional: also link C++ runtime if using C++ (e.g., OpenCV)
 //   println!("cargo:rustc-link-lib=dylib=c++");

}
