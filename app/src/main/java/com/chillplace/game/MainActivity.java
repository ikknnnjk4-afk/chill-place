package com.chillplace.game;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.StringWriter;

/**
 * Pure Java launcher (NO native code).
 * - If a crash file exists → show it full screen (screenshot this).
 * - Otherwise → start the native game activity.
 * This works even when the 3D engine always crashes on launch.
 */
public class MainActivity extends Activity {
    private static final String TAG = "ChillPlace";
    private static final String CRASH_NAME = "chillplace_crash.txt";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "Java MainActivity.onCreate (launcher)");

        // Capture Java crashes too
        final Thread.UncaughtExceptionHandler prev =
                Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((t, e) -> {
            writeCrash("JAVA", stackTrace(e));
            if (prev != null) prev.uncaughtException(t, e);
            else System.exit(2);
        });

        String crash = readCrash();
        if (crash != null && !crash.isEmpty()) {
            showCrashScreen(crash);
        } else {
            // No previous crash → go straight to the game
            startGame();
        }
    }

    private void showCrashScreen(String crash) {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.rgb(30, 10, 10));
        root.setPadding(24, 40, 24, 24);

        TextView title = new TextView(this);
        title.setText("CRASH DETECTE\nEnvoie une capture a Grok");
        title.setTextColor(Color.RED);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        title.setGravity(Gravity.CENTER);
        root.addView(title);

        ScrollView scroll = new ScrollView(this);
        TextView body = new TextView(this);
        body.setText(crash);
        body.setTextColor(Color.WHITE);
        body.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        body.setPadding(8, 16, 8, 16);
        body.setTextIsSelectable(true);
        scroll.addView(body);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
        root.addView(scroll, sp);

        Button copy = new Button(this);
        copy.setText("Copier le rapport");
        copy.setOnClickListener(v -> {
            android.content.ClipboardManager cm =
                    (android.content.ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
            cm.setPrimaryClip(android.content.ClipData.newPlainText("crash", crash));
            Toast.makeText(this, "Copie dans le presse-papiers", Toast.LENGTH_SHORT).show();
        });
        root.addView(copy);

        Button launch = new Button(this);
        launch.setText("Lancer le jeu quand meme");
        launch.setOnClickListener(v -> startGame());
        root.addView(launch);

        Button clear = new Button(this);
        clear.setText("Effacer le rapport");
        clear.setOnClickListener(v -> {
            deleteCrash();
            Toast.makeText(this, "Rapport efface", Toast.LENGTH_SHORT).show();
            startGame();
        });
        root.addView(clear);

        setContentView(root);
    }

    private void startGame() {
        Log.i(TAG, "Starting native GameActivity");
        try {
            Intent i = new Intent(this, GameActivity.class);
            startActivity(i);
            // Don't finish() immediately – if game crashes instantly user can go back
        } catch (Throwable t) {
            writeCrash("LAUNCH", stackTrace(t));
            showCrashScreen(readCrash());
        }
    }

    private String readCrash() {
        StringBuilder sb = new StringBuilder();
        File[] candidates = {
                new File(getFilesDir(), CRASH_NAME),
                new File(getCacheDir(), CRASH_NAME)
        };
        for (File f : candidates) {
            if (f == null || !f.exists()) continue;
            try (BufferedReader br = new BufferedReader(new FileReader(f))) {
                String line;
                while ((line = br.readLine()) != null) {
                    sb.append(line).append('\n');
                }
            } catch (Exception e) {
                Log.w(TAG, "readCrash failed", e);
            }
            if (sb.length() > 0) return sb.toString();
        }
        return null;
    }

    private void deleteCrash() {
        new File(getFilesDir(), CRASH_NAME).delete();
        new File(getCacheDir(), CRASH_NAME).delete();
    }

    static void writeCrash(String kind, String detail) {
        // Called from exception handler – keep it simple
        try {
            // Can't always get context; try app paths via reflection-free known path
            String report = "=== CHILL PLACE CRASH (" + kind + ") ===\n" + detail + "\n--- end ---\n";
            File dir = new File("/data/data/com.chillplace.game/files");
            // no-op if dirs missing; GameActivity / native also write here
            dir.mkdirs();
            try (FileWriter fw = new FileWriter(new File(dir, CRASH_NAME), false)) {
                fw.write(report);
            }
            File cache = new File("/data/data/com.chillplace.game/cache");
            cache.mkdirs();
            try (FileWriter fw = new FileWriter(new File(cache, CRASH_NAME), false)) {
                fw.write(report);
            }
            Log.e(TAG, "Crash written (" + kind + ")");
        } catch (Throwable ignored) {
        }
    }

    private static String stackTrace(Throwable e) {
        StringWriter sw = new StringWriter();
        e.printStackTrace(new PrintWriter(sw));
        return sw.toString();
    }
}
