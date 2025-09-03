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
