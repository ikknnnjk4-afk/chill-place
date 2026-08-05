package com.chillplace.game;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.StringWriter;

/**
 * NativeActivity + Java crash capture.
 * Writes uncaught Java exceptions next to the native crash file
 * so the next launch can show them on screen.
 */
public class MainActivity extends NativeActivity {
    private static final String TAG = "ChillPlace";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "MainActivity.onCreate");

        // Capture ANY uncaught Java exception
        final Thread.UncaughtExceptionHandler previous =
                Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
            try {
                StringWriter sw = new StringWriter();
                throwable.printStackTrace(new PrintWriter(sw));
                String report =
                        "=== CHILL PLACE JAVA CRASH ===\n" +
                        "thread=" + thread.getName() + "\n" +
                        sw.toString() +
                        "--- end ---\n";
                File dir = getFilesDir();
                if (dir != null) {
                    File out = new File(dir, "chillplace_crash.txt");
                    try (FileWriter fw = new FileWriter(out, false)) {
                        fw.write(report);
                    }
                    Log.e(TAG, "Java crash written to " + out.getAbsolutePath());
                }
                // Also try cache
                File cache = getCacheDir();
                if (cache != null) {
                    try (FileWriter fw = new FileWriter(new File(cache, "chillplace_crash.txt"), false)) {
                        fw.write(report);
                    }
                }
            } catch (Throwable ignored) {
            }
            if (previous != null) previous.uncaughtException(thread, throwable);
            else System.exit(2);
        });

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
