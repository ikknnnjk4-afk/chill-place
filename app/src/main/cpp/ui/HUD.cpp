#include "HUD.h"
#include "raylib.h"
#include "raymath.h"
#include <cstring>
#include <jni.h>
#include <android/log.h>

// From GroqClient.cpp — polls pending AI responses on main thread
extern "C" void GroqClient_PollPending();

// JavaVM stored by JNI_OnLoad in GroqClient.cpp
extern JavaVM* gJavaVM;

// API key injected at build time via BuildConfig — accessed via JNI helper
static std::string GetGroqApiKey() {
    if (!gJavaVM) return "";

    JNIEnv* env = nullptr;
    bool attached = false;
    if (gJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (gJavaVM->AttachCurrentThread(&env, nullptr) != 0 || !env) return "";
        attached = true;
    }
    if (!env) return "";

    std::string key;
    jclass cls = env->FindClass("com/chillplace/game/BuildConfig");
    if (env->ExceptionCheck()) { env->ExceptionClear(); cls = nullptr; }
    if (cls) {
        jfieldID fid = env->GetStaticFieldID(cls, "GROQ_API_KEY", "Ljava/lang/String;");
        if (env->ExceptionCheck()) { env->ExceptionClear(); fid = nullptr; }
        if (fid) {
            jstring jkey = reinterpret_cast<jstring>(env->GetStaticObjectField(cls, fid));
            if (env->ExceptionCheck()) { env->ExceptionClear(); jkey = nullptr; }
            if (jkey) {
                const char* ckey = env->GetStringUTFChars(jkey, nullptr);
                if (ckey) {
                    key = ckey;
                    env->ReleaseStringUTFChars(jkey, ckey);
                }
                env->DeleteLocalRef(jkey);
            }
        }
        env->DeleteLocalRef(cls);
    }

    if (attached) gJavaVM->DetachCurrentThread();
    return key;
}

void HUD::Load() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (sw <= 0) sw = 1280;
    if (sh <= 0) sh = 720;

    // Virtual joystick on the left
    joyCenter = { (float)(sw * 0.13f), (float)(sh * 0.75f) };
    joyPos    = joyCenter;

    // Init Groq client (safe – never crashes the game if key missing)
    apiKey = GetGroqApiKey();
    if (!apiKey.empty()) {
        groq = std::make_unique<GroqClient>(apiKey);
    }
}

void HUD::Update(float dt) {
    GroqClient_PollPending();
    UpdateJoystick();
    UpdateLookTouch();

    // Check chat button tap
    chatButtonTap = false;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Rectangle chatBtn = { (float)(sw - 90), (float)(sh - 90), 80, 80 };

    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 tp = GetTouchPosition(i);
        if (CheckCollisionPointRec(tp, chatBtn)) {
            if (IsGestureDetected(GESTURE_TAP)) chatButtonTap = true;
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mp = GetMousePosition();
        if (CheckCollisionPointRec(mp, chatBtn)) chatButtonTap = true;
    }
}

void HUD::UpdateJoystick() {
    int sw = GetScreenWidth();
    const float radius = sw * 0.08f;

    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 tp = GetTouchPosition(i);
        float dist = Vector2Distance(tp, joyCenter);

        if (!joyActive && dist < radius * 1.5f) {
            joyActive  = true;
            joyTouchId = i;
        }
        if (joyActive && i == joyTouchId) {
            Vector2 delta = Vector2Subtract(tp, joyCenter);
            float d = Vector2Length(delta);
            if (d > radius) {
                delta = Vector2Scale(Vector2Normalize(delta), radius);
            }
            joyPos = Vector2Add(joyCenter, delta);
        }
    }
    if (GetTouchPointCount() == 0) {
        joyActive  = false;
        joyTouchId = -1;
        joyPos     = joyCenter;
    }
}

void HUD::UpdateLookTouch() {
    int sw = GetScreenWidth();

    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 tp = GetTouchPosition(i);
        if (tp.x > sw * 0.3f) { // Right half → look
            if (!lookActive) {
                lookActive  = true;
                lookTouchId = i;
                lookPrev    = tp;
            } else if (i == lookTouchId) {
                lookPrev = tp;
            }
        }
    }
    if (GetTouchPointCount() == 0) {
        lookActive  = false;
        lookTouchId = -1;
    }
}

bool HUD::ChatButtonPressed() const { return chatButtonTap; }

void HUD::OpenChat() {
    chatOpen     = false;
    chatCloseReq = false;
    inputBuffer.clear();
    chatScrollY = 0.0f;
}

void HUD::UpdateChat(float dt) {
    GroqClient_PollPending();
    chatCloseReq = false;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Close button (top-right)
    Rectangle closeBtn = { (float)(sw - 60), 10, 50, 50 };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mp = GetMousePosition();
        if (CheckCollisionPointRec(mp, closeBtn)) chatCloseReq = true;
    }
    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 tp = GetTouchPosition(i);
        if (CheckCollisionPointRec(tp, closeBtn) && IsGestureDetected(GESTURE_TAP))
            chatCloseReq = true;
    }

    // Text input via keyboard (Android soft keyboard triggered by SetActiveGestures)
    int key;
    while ((key = GetCharPressed()) != 0) {
        if (key >= 32 && key <= 125 && inputBuffer.size() < 200) {
            inputBuffer += (char)key;
        }
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !inputBuffer.empty()) {
        inputBuffer.pop_back();
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        SendChatMessage();
    }

    // Send button tap
    Rectangle sendBtn = { (float)(sw - 110), (float)(sh - 70), 100, 55 };
    bool sendTap = false;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mp = GetMousePosition();
        if (CheckCollisionPointRec(mp, sendBtn)) sendTap = true;
    }
    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 tp = GetTouchPosition(i);
        if (CheckCollisionPointRec(tp, sendBtn) && IsGestureDetected(GESTURE_TAP))
            sendTap = true;
    }
    if (sendTap) SendChatMessage();
}

void HUD::SendChatMessage() {
    if (inputBuffer.empty() || !groq || groq->IsBusy()) return;

    std::string msg = inputBuffer;
    inputBuffer.clear();
    chatHistory.push_back({ msg, true });
    waiting = true;

    groq->SendAsync(msg, [this](const std::string& reply) {
        chatHistory.push_back({ reply, false });
        waiting = false;
    });
}

bool HUD::ChatClosed() const { return chatCloseReq; }

void HUD::Draw() const {
    DrawJoystick();
    DrawChatButton();

    // FPS debug (top-left, small)
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, { 255, 255, 255, 120 });
}

void HUD::DrawJoystick() const {
    int sw = GetScreenWidth();
    const float radius = sw * 0.08f;
    Color outerC = { 255, 255, 255, 50 };
    Color innerC = { 255, 255, 255, 120 };
    DrawCircleV(joyCenter, radius,      outerC);
    DrawCircleV(joyPos,    radius*0.4f, innerC);
}

void HUD::DrawChatButton() const {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Rectangle r = { (float)(sw - 90), (float)(sh - 90), 80, 80 };
    DrawRectangleRounded(r, 0.3f, 8, { 80, 60, 120, 200 });
    DrawText("AI", (int)r.x + 22, (int)r.y + 22, 28, { 255, 230, 100, 255 });
}

void HUD::DrawChat() const {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Semi-transparent panel
    DrawRectangle(0, 0, sw, sh, { 0, 0, 0, 180 });
    DrawRectangleRounded({ 10, 10, (float)(sw - 20), (float)(sh - 20) }, 0.04f, 8,
                         { 20, 15, 40, 230 });

    // Title
    DrawText("Mocho — Chat Chill Place", 30, 20, 24, { 255, 220, 100, 255 });

    // Close button
    DrawText("X", sw - 48, 18, 32, { 255, 100, 100, 255 });

    // Chat history
    int y = 60;
    for (auto& line : chatHistory) {
        Color col = line.isUser ? (Color){180, 220, 255, 255}
                                : (Color){200, 255, 180, 255};
        const char* prefix = line.isUser ? "Toi: " : "Mocho: ";
        std::string full = prefix + line.text;
        DrawText(full.c_str(), 20, y, 18, col);
        y += 28;
        if (y > sh - 160) break;
    }

    // Thinking indicator
    if (waiting) {
        DrawText("Mocho réfléchit...", 20, y, 18, { 180, 180, 180, 200 });
    }

    // Input field
    Rectangle inputRect = { 10, (float)(sh - 75), (float)(sw - 130), 60 };
    DrawRectangleRounded(inputRect, 0.2f, 8, { 40, 30, 60, 255 });
    std::string display = inputBuffer + "|";
    DrawText(display.c_str(), (int)inputRect.x + 10, (int)inputRect.y + 15, 20,
             { 255, 255, 255, 255 });

    // Send button
    Rectangle sendBtn = { (float)(sw - 110), (float)(sh - 70), 100, 55 };
    DrawRectangleRounded(sendBtn, 0.3f, 8, { 80, 160, 80, 255 });
    DrawText("Envoyer", (int)sendBtn.x + 5, (int)sendBtn.y + 15, 18, WHITE);
}

void HUD::Unload() {
    groq.reset();
}
