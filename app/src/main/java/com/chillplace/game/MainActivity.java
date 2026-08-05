package com.chillplace.game;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

/**
 * Custom NativeActivity for better HyperOS / Xiaomi compatibility.
 * Pure android.app.NativeActivity often crashes on launch on Redmi devices.
 */
public class MainActivity extends NativeActivity {
    private static final String TAG = "ChillPlace";

    static {
        // Load the native library early and explicitly
        try {
            System.loadLibrary("game");
            Log.i(TAG, "libgame.so loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load libgame.so", e);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "MainActivity.onCreate");
        // Keep screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onResume() {
        Log.i(TAG, "MainActivity.onResume");
        super.onResume();
    }

    @Override
    protected void onPause() {
        Log.i(TAG, "MainActivity.onPause");
        super.onPause();
    }
}
