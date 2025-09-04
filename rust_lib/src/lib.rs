use std::ffi::{CStr, CString};
use opencv::prelude::*;
use opencv::{highgui, imgcodecs};

#[macro_use] extern crate log;
extern crate android_log;
// use yolo_detector::YoloDetector;

#[unsafe(no_mangle)]
pub extern "C" fn rust_opencv_test() {
//    let detector = YoloDetector::new("yolov8m.onnx", "coco.names", 640).unwrap();
    android_log::init("NativeOpenGL").unwrap();

    match imgcodecs::imread("image.jpg", imgcodecs::IMREAD_COLOR) {
        Ok(mat) => info!("Image loaded"),
        Err(e) => error!("Error loading image: {}", e),
    }
    // if let Some(mat) = imgcodecs::imread("image.jpg", imgcodecs::IMREAD_COLOR) {
    //     println!("Image loaded successfully!");
    // } else {
    //     println!("Failed to load image!");
    // }

 //   let (detections, original_size) = detector.detect(&mat.clone())?;

  //  let result = detector.draw_detections(mat.clone(), detections, 0.5, 0.5, original_size)?;
//    println!(mat.);

//    highgui::imshow("YOLOv8 Video", &result)?;
    //highgui::wait_key(0);
}

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
