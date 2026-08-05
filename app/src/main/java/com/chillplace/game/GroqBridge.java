package com.chillplace.game;

import android.util.Log;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import org.json.JSONArray;
import org.json.JSONObject;

/**
 * GroqBridge — Called from C++ via JNI.
 * Makes HTTP requests to the Groq API and returns the response text.
 */
public class GroqBridge {

    private static final String TAG = "GroqBridge";
    private static final String GROQ_URL = "https://api.groq.com/openai/v1/chat/completions";
    private static final String MODEL = "llama3-8b-8192";

    /**
     * Send a message to Groq AI and get a response.
     * Called from C++ via JNI.
     *
     * @param apiKey   Groq API key
     * @param userMsg  The user's message
     * @param history  JSON array string of previous messages
     * @return         AI response text, or "ERREUR: ..." on failure
     */
    public static String sendMessage(String apiKey, String userMsg, String history) {
        try {
            // Build messages array
            JSONArray messages = new JSONArray();

            // System prompt
            JSONObject system = new JSONObject();
            system.put("role", "system");
            system.put("content",
                "Tu es Mocho, un chat relaxant qui vit dans un salon cosy appelé Chill Place. " +
                "Tu parles français, tu es chaleureux, amusant et philosophique. " +
                "Tu aides les gens à se détendre et à réfléchir. " +
                "Tes réponses sont courtes (2-3 phrases max).");
            messages.put(system);

            // Previous messages from history
            if (history != null && !history.isEmpty()) {
                try {
                    JSONArray prev = new JSONArray(history);
                    for (int i = 0; i < prev.length(); i++) {
                        messages.put(prev.getJSONObject(i));
                    }
                } catch (Exception e) {
                    Log.w(TAG, "Could not parse history: " + e.getMessage());
                }
            }

            // New user message
            JSONObject userMessage = new JSONObject();
            userMessage.put("role", "user");
            userMessage.put("content", userMsg);
            messages.put(userMessage);

            // Build request body
            JSONObject body = new JSONObject();
            body.put("model", MODEL);
            body.put("messages", messages);
            body.put("max_tokens", 150);
            body.put("temperature", 0.8);

            String bodyStr = body.toString();

            // Make HTTP request
            URL url = new URL(GROQ_URL);
            HttpURLConnection conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Authorization", "Bearer " + apiKey);
            conn.setRequestProperty("Content-Type", "application/json");
            conn.setRequestProperty("Accept", "application/json");
            conn.setConnectTimeout(10000);
            conn.setReadTimeout(30000);
            conn.setDoOutput(true);

            try (OutputStream os = conn.getOutputStream()) {
                byte[] input = bodyStr.getBytes(StandardCharsets.UTF_8);
                os.write(input, 0, input.length);
            }

            int responseCode = conn.getResponseCode();
            BufferedReader reader;
            if (responseCode == 200) {
                reader = new BufferedReader(new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
            } else {
                reader = new BufferedReader(new InputStreamReader(conn.getErrorStream(), StandardCharsets.UTF_8));
            }

            StringBuilder response = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                response.append(line);
            }
            reader.close();

            if (responseCode == 200) {
                JSONObject resp = new JSONObject(response.toString());
                return resp.getJSONArray("choices")
                           .getJSONObject(0)
                           .getJSONObject("message")
                           .getString("content")
                           .trim();
            } else {
                Log.e(TAG, "Groq error " + responseCode + ": " + response);
                return "ERREUR: " + responseCode;
            }

        } catch (Exception e) {
            Log.e(TAG, "Exception in sendMessage: " + e.getMessage());
            return "ERREUR: " + e.getMessage();
        }
    }
}
