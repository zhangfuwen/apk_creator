use std::ffi::{CStr, CString};


pub fn add(left: u64, right: u64) -> u64 {
    left + right
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = add(2, 2);
        assert_eq!(result, 4);
    }
}

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
