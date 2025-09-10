package com.example.app;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.util.Log;
import android.Manifest;
import android.content.pm.PackageManager;
import android.content.Context;
import android.hardware.camera2.CameraManager;
import java.util.concurrent.Executors;
import android.widget.Toast;

public class MainActivity extends Activity {
    private CameraManager cameraManager;
    private String cameraId;

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

    private class CameraDeviceStateCallback extends android.hardware.camera2.CameraDevice.StateCallback {
        @Override
        public void onOpened(android.hardware.camera2.CameraDevice camera) {
            Log.d("NativeOpenGL", "Camera opened");
//            Toast.makeText(MainActivity.this, "Camera opened!", Toast.LENGTH_SHORT).show();
            // Here you would set up a CaptureSession and Surface for preview
            // This is more complex but gives full control
        }

        @Override
        public void onDisconnected(android.hardware.camera2.CameraDevice camera) {
            camera.close();
        }

        @Override
        public void onError(android.hardware.camera2.CameraDevice camera, int error) {
            Log.e("NativeOpenGL", "Camera Error");
            camera.close();
        }
    }

    void requestCameraPermission() {
        this.requestPermissions(new String[]{"android.permission.CAMERA"}, 5);
    }
}
