package com.example.app;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.util.Log;
import android.Manifest;
import android.content.pm.PackageManager;
import android.content.Context;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.os.Bundle;
import android.os.Environment;
import java.util.concurrent.Executors;
import android.widget.Toast;
import android.media.ImageReader;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CaptureRequest;
import android.media.Image;
import android.os.Handler;
import android.os.Looper;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;

public class MainActivity extends Activity {
    private CameraManager cameraManager;
    private CameraDevice cameraDevice;
    private String cameraId = "0";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView tv = new TextView(this);
        tv.setText("Hello from APK built by hand!");
        setContentView(tv);

        cameraManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);

        Log.d("NativeOpenGL", "Hello from APK built by hand!");
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == 5) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Log.d("NativeOpenGL", "Camera permission granted");
            } else {
                Log.d("NativeOpenGL", "Camera permission denied");
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (this.checkSelfPermission(Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            requestCameraPermission();
        } else {
            Log.d("NativeOpenGL", "Camera permission already granted");
        }
        try {
            // Get first rear-facing camera
            for (String id : cameraManager.getCameraIdList()) {
                if (cameraManager.getCameraCharacteristics(id)
                        .get(android.hardware.camera2.CameraCharacteristics.LENS_FACING)
                        == android.hardware.camera2.CameraCharacteristics.LENS_FACING_BACK) {
                    cameraId = id;
                    break;
                }
            }

            if (cameraId == null) {
                Toast.makeText(this, "No back camera found", Toast.LENGTH_SHORT).show();
                finish();
                return;
            }

            // Open camera (you'd normally set up preview surface, etc.)
            cameraManager.openCamera(cameraId,  Executors.newSingleThreadExecutor(), new CameraDeviceStateCallback());

        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(this, "Error opening camera", Toast.LENGTH_SHORT).show();
        }
    }

     private void takePicture() {
        try {
            // Create ImageReader for JPEG (width=640, height=480, max 1 image)
            final ImageReader imageReader = android.media.ImageReader.newInstance(640, 480, android.graphics.ImageFormat.JPEG, 1);
            imageReader.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() {
                @Override
                public void onImageAvailable(ImageReader reader) {
                    Image image = null;
                    try {
                        image = reader.acquireLatestImage();
                        if (image == null) return;

                        ByteBuffer buffer = image.getPlanes()[0].getBuffer();
                        byte[] bytes = new byte[buffer.remaining()];
                        buffer.get(bytes);

                        File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
                        dir.mkdirs(); // Ensure directory exists
                        File file = new File(dir, "photo.jpg");
                        FileOutputStream fos = new FileOutputStream(file);
                        fos.write(bytes);
                        fos.close();

                        Toast.makeText(MainActivity.this, "Photo saved!", Toast.LENGTH_SHORT).show();
                    } catch (IOException e) {
                        e.printStackTrace();
                    } finally {
                        if (image != null) {
                            image.close();
                        }
                        imageReader.close();
                        cameraDevice.close();
                    }
                }
            }, new Handler(Looper.getMainLooper()));

            // Start a capture session
            cameraDevice.createCaptureSession(
                Arrays.asList(imageReader.getSurface()),
                new CameraCaptureSession.StateCallback() {
                    @Override
                    public void onConfigured(CameraCaptureSession session) {
                        try {
                            CaptureRequest.Builder builder =
                                cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
                            builder.addTarget(imageReader.getSurface());
                            session.capture(builder.build(), null, null);
                        } catch (Exception e) {
                            Log.e("NativeOpenGL", "Failed to capture", e);
                            cameraDevice.close();
                        }
                    }

                    @Override
                    public void onConfigureFailed(CameraCaptureSession session) {
                        Log.e("NativeOpenGL", "Capture session failed");
                        cameraDevice.close();
                    }
                },
                new Handler(Looper.getMainLooper()) 
            );

        } catch (Exception e) {
            Log.e("NativeOpenGL", "Failed to take picture", e);
            cameraDevice.close();
        }
    }

    private class CameraDeviceStateCallback extends android.hardware.camera2.CameraDevice.StateCallback {
        @Override
        public void onOpened(android.hardware.camera2.CameraDevice camera) {
            Log.d("NativeOpenGL", "Camera opened");
//            Toast.makeText(MainActivity.this, "Camera opened!", Toast.LENGTH_SHORT).show();
            // Here you would set up a CaptureSession and Surface for preview
            // This is more complex but gives full control
            cameraDevice = camera;
            takePicture();  // Take picture as soon as opened
        }

        @Override
        public void onDisconnected(android.hardware.camera2.CameraDevice camera) {
            camera.close();
            cameraDevice = null;
        }

        @Override
        public void onError(android.hardware.camera2.CameraDevice camera, int error) {
            Log.e("NativeOpenGL", "Camera Error");
            camera.close();
            cameraDevice = null;
        }
    }

    void requestCameraPermission() {
        this.requestPermissions(new String[]{"android.permission.CAMERA"}, 5);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (cameraDevice != null) {
            cameraDevice.close();
            cameraDevice = null;
        }
    }
}
