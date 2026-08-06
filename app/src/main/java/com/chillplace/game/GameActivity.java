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
 * Native 3D game. Crashes here are written to chillplace_crash.txt
 * so the pure-Java MainActivity can show them next time.
 */
public class GameActivity extends NativeActivity {
    private static final String TAG = "ChillPlace";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "GameActivity.onCreate (native)");

        final Thread.UncaughtExceptionHandler prev =
                Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((t, e) -> {
            try {
                StringWriter sw = new StringWriter();
                e.printStackTrace(new PrintWriter(sw));
                String report = "=== CHILL PLACE JAVA CRASH (GameActivity) ===\n"
                        + "thread=" + t.getName() + "\n" + sw + "--- end ---\n";
                File dir = getFilesDir();
                if (dir != null) {
                    try (FileWriter fw = new FileWriter(new File(dir, "chillplace_crash.txt"), false)) {
                        fw.write(report);
                    }
                }
                File cache = getCacheDir();
                if (cache != null) {
                    try (FileWriter fw = new FileWriter(new File(cache, "chillplace_crash.txt"), false)) {
                        fw.write(report);
                    }
                }
                Log.e(TAG, "Native-side Java crash written");
            } catch (Throwable ignored) {
            }
            if (prev != null) prev.uncaughtException(t, e);
            else System.exit(2);
        });

        try {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } catch (Throwable ignored) {
        }
        super.onCreate(savedInstanceState);
        Log.i(TAG, "GameActivity.onCreate done");
    }
}
