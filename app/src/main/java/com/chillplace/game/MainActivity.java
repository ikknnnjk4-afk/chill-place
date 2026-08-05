package com.chillplace.game;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

/**
 * Thin NativeActivity subclass for HyperOS / Xiaomi stability.
 * Do NOT call System.loadLibrary here – NativeActivity loads via
 * android.app.lib_name meta-data. Double-loading the .so crashes
 * on some devices.
 */
public class MainActivity extends NativeActivity {
    private static final String TAG = "ChillPlace";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "MainActivity.onCreate");
        try {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } catch (Throwable t) {
            Log.w(TAG, "KEEP_SCREEN_ON failed", t);
        }
        super.onCreate(savedInstanceState);
        Log.i(TAG, "MainActivity.onCreate done");
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

    @Override
    protected void onDestroy() {
        Log.i(TAG, "MainActivity.onDestroy");
        super.onDestroy();
    }
}
