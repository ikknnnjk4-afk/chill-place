package com.chillplace.game;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;

/**
 * Pure Java launcher – NEVER loads native code.
 * If a previous crash exists, shows it so the user can screenshot
 * and send to the developer. Then a button starts the real game.
 */
public class LauncherActivity extends Activity {
    private static final String TAG = "ChillPlace";
    private static final String CRASH_NAME = "chillplace_crash.txt";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "LauncherActivity.onCreate");

        // Capture Java crashes from this activity too
        Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
            try {
                writeCrash("JAVA: " + android.util.Log.getStackTraceString(throwable));
            } catch (Throwable ignored) {}
            System.exit(2);
        });

        String crash = readCrash();

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.rgb(18, 12, 12));
        root.setPadding(dp(16), dp(24), dp(16), dp(16));

        TextView title = new TextView(this);
        title.setText(crash.isEmpty()
                ? "Chill Place"
                : "CRASH DETECTE – envoie une capture a Grok");
        title.setTextColor(crash.isEmpty() ? Color.rgb(255, 230, 100) : Color.RED);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setPadding(0, 0, 0, dp(12));
        root.addView(title);

        if (!crash.isEmpty()) {
            TextView hint = new TextView(this);
            hint.setText("Fais une capture d'ecran de ce texte et envoie-la.");
            hint.setTextColor(Color.LTGRAY);
            hint.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
            hint.setPadding(0, 0, 0, dp(8));
            root.addView(hint);

            ScrollView scroll = new ScrollView(this);
            LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
            scroll.setLayoutParams(sp);

            TextView body = new TextView(this);
            body.setText(crash);
            body.setTextColor(Color.WHITE);
            body.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
            body.setTypeface(Typeface.MONOSPACE);
            body.setTextIsSelectable(true); // long-press to copy
            scroll.addView(body);
            root.addView(scroll);

            Button clear = new Button(this);
            clear.setText("Effacer le rapport");
            clear.setOnClickListener(v -> {
                deleteCrash();
                Toast.makeText(this, "Rapport efface", Toast.LENGTH_SHORT).show();
                recreate();
            });
            root.addView(clear);
        } else {
            TextView ok = new TextView(this);
            ok.setText("Aucun crash precedent.\nAppuie sur JOUER pour lancer.");
            ok.setTextColor(Color.LTGRAY);
            ok.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
            ok.setPadding(0, 0, 0, dp(24));
            root.addView(ok);

            // spacer
            LinearLayout.LayoutParams flex = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
            TextView spacer = new TextView(this);
            spacer.setLayoutParams(flex);
            root.addView(spacer);
        }

        Button play = new Button(this);
        play.setText(crash.isEmpty() ? "JOUER" : "Relancer le jeu quand meme");
        play.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        play.setOnClickListener(v -> {
            Log.i(TAG, "Starting MainActivity (native)");
            try {
                Intent i = new Intent(this, MainActivity.class);
                startActivity(i);
            } catch (Throwable t) {
                writeCrash("Failed to start MainActivity:\n" + Log.getStackTraceString(t));
                Toast.makeText(this, "Echec lancement – relance l'app pour voir le rapport", Toast.LENGTH_LONG).show();
            }
        });
        root.addView(play);

        TextView foot = new TextView(this);
        foot.setText("v1.6 – launcher Java (sans code natif)");
        foot.setTextColor(Color.GRAY);
        foot.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        foot.setGravity(Gravity.CENTER);
        foot.setPadding(0, dp(8), 0, 0);
        root.addView(foot);

        setContentView(root);
    }

    private int dp(int v) {
        float d = getResources().getDisplayMetrics().density;
        return Math.round(v * d);
    }

    private File crashFile() {
        File dir = getFilesDir();
        if (dir == null) dir = getCacheDir();
        return new File(dir, CRASH_NAME);
    }

    private String readCrash() {
        try {
            File f = crashFile();
            if (!f.exists()) {
                // also try cache
                File c = new File(getCacheDir(), CRASH_NAME);
                if (c.exists()) f = c;
                else return "";
            }
            StringBuilder sb = new StringBuilder();
            try (BufferedReader br = new BufferedReader(new FileReader(f))) {
                String line;
                while ((line = br.readLine()) != null) {
                    sb.append(line).append('\n');
                }
            }
            return sb.toString().trim();
        } catch (Throwable t) {
            return "Erreur lecture crash: " + t.getMessage();
        }
    }

    private void writeCrash(String text) {
        try {
            File f = crashFile();
            try (FileWriter fw = new FileWriter(f, false)) {
                fw.write("=== CHILL PLACE CRASH ===\n");
                fw.write(text);
                fw.write("\n--- end ---\n");
            }
            Log.e(TAG, "Crash written to " + f.getAbsolutePath());
        } catch (Throwable t) {
            Log.e(TAG, "writeCrash failed", t);
        }
    }

    private void deleteCrash() {
        try {
            File f = crashFile();
            if (f.exists()) f.delete();
            File c = new File(getCacheDir(), CRASH_NAME);
            if (c.exists()) c.delete();
        } catch (Throwable ignored) {}
    }
}
