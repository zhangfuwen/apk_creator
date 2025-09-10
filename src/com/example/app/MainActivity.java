package com.example.app;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.util.Log;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView tv = new TextView(this);
        tv.setText("Hello from APK built by hand!");
        setContentView(tv);

        Log.d("NativeOpenGL", "Hello from APK built by hand!");

        requestCameraPermission();
    }

    void requestCameraPermission() {
        this.requestPermissions(new String[]{"android.permission.CAMERA"}, 5);
    }
}
