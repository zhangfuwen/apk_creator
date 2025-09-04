# rust lib

> [!WARNING] All code are run under project root

## Setup environment

```bash
source .ndk_env
```

## Setup rust

```bash
cargo install cargo-ndk
rustup target add aarch64-linux-android armv7-linux-androideabi # x86_64-linux-android

```

## Creating a project

```bash
cargo new --lib rust_lib

```


## Build the code

```bash
cd rust_lib
cargo ndk -t arm64-v8a -t armeabi-v7a build --release

```

## Use shared library

Modify Cargo.toml:

```toml
[package]
name = "rust_lib"
version = "0.1.0"
edition = "2024"

[lib]
name = "rust_lib"
crate-type = ["cdylib"]

[dependencies]
```

## export C symbol

Modify src/lib.rs, use `#[no_mangle]` and `pub extern "C" fn`:

```rust
use std::ffi::{CStr, CString};

#[unsafe(no_mangle)]
pub extern "C" fn rust_greeting(from: *const u8) -> *mut u8 {
    use std::ffi::{CStr, CString};
    let from_rust = unsafe {
        CStr::from_ptr(from)
    }.to_string_lossy()
     .into_owned();

    let result = format!("Hello, {}! (from Rust 🦀)", from_rust);
    CString::new(result)
        .expect("CString::new failed")
        .into_raw()
}

// Optional: cleanup function to avoid memory leaks
#[unsafe(no_mangle)]
pub extern "C" fn rust_free(s: *mut u8) {
    if s.is_null() { return }
    unsafe {
        let _ = CString::from_raw(s);
        // Just drop it to free memory
    }
}
```

build and then you will get:

```text
> readelf -sW target/aarch64-linux-android/release/librust_lib.so | grep rust_
  ...
  2643: 000000000001807c    44 FUNC    GLOBAL DEFAULT   14 rust_free
  2644: 0000000000017e70   524 FUNC    GLOBAL DEFAULT   14 rust_greeting
  ...
```


## Using OpenCV via vcpkg

### Install C++ opencv4 using vcpkg

```bash

export VCPKG_ROOT="$HOME/bin/vcpkg"
[[ -d $VCPKG_ROOT ]] ||  git clone https://github.com/microsoft/vcpkg "$HOME/bin/vcpkg"
export PATH=$PATH:$VCPKG_ROOT/
export VCPKG_DEFAULT_TRIPLET=arm64-android
export EDITOR=nvim

# rust specific env vars
export VCPKGRS_TRIPLET=arm64-android

vcpkg install opencv4:arm64-android

```

### Use opencv in rust code 

```rust

use opencv::prelude::*;
use opencv::{highgui, imgcodecs};

#[unsafe(no_mangle)]
pub extern "C" fn rust_opencv_test() {

    match imgcodecs::imread("image.jpg", imgcodecs::IMREAD_COLOR) {
        Ok(mat) => println!("Image loaded"),
        Err(e) => println!("Error loading image: {}", e),
    }
    highgui::wait_key(0);
}

```

### Build the rust library

```bash
# cargo install cargo-vcpkg

# this command will download vcpkg and 
# install dependencies declared under the vcpkg section
# cargo vcpkg build

# checkout to lastest tag manually
# so that vcpkg support arm64-android

# this command will build rust and find dependencies via vcpkg
cargo ndk -t arm64-v8a -t armeabi-v7a build --release

```

### Copy library to APK build directory 

```bash
mkdir build/rust_lib/lib/arm64-v8a
cp rust_lib/target/aarch64-linux-android/release/librust_lib.so build/rust_lib/lib/arm64-v8a/
```
