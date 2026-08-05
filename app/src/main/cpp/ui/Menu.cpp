#include "Menu.h"
#include "raylib.h"
#include <cmath>
#include <android/log.h>

#define LOG_TAG "ChillPlace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

void Menu::Load() {
    LOGI("Menu::Load");
    texTitle = LoadTexture("ui/titre_ecran_menu_principal.png");
    LOGI("Menu title texture id=%u", texTitle.id);
    if (texTitle.id > 0) {
        SetTextureFilter(texTitle, TEXTURE_FILTER_BILINEAR);
    }
    loaded   = true;
    fadeAlpha = 0.0f;
    pulse     = 0.0f;
}

bool Menu::Update() {
    float dt = GetFrameTime();
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;

    if (fadeAlpha < 1.0f) {
        fadeAlpha += dt * 0.8f;
        if (fadeAlpha > 1.0f) fadeAlpha = 1.0f;
    }

    pulse += dt * 2.0f;

    int sh = GetScreenHeight();
    if (sh <= 0) return false;

    bool tapped = false;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 pos = GetMousePosition();
        if (pos.y > sh * 0.6f) tapped = true;
    }
    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 tp = GetTouchPosition(i);
        if (tp.y > sh * 0.6f) tapped = true;
    }

    return tapped && fadeAlpha >= 0.9f;
}

void Menu::Draw() const {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (sw <= 0 || sh <= 0) return;

    if (loaded && texTitle.id > 0) {
        float scaleX = (float)sw / (float)texTitle.width;
        float scaleY = (float)sh / (float)texTitle.height;
        float scale  = (scaleX > scaleY) ? scaleX : scaleY;
        int dw = (int)(texTitle.width  * scale);
        int dh = (int)(texTitle.height * scale);
        int ox = (sw - dw) / 2;
        int oy = (sh - dh) / 2;
        DrawTexturePro(texTitle,
            { 0, 0, (float)texTitle.width, (float)texTitle.height },
            { (float)ox, (float)oy, (float)dw, (float)dh },
            { 0, 0 }, 0.0f,
            Fade(WHITE, fadeAlpha));
    } else {
        DrawRectangle(0, 0, sw, sh, { 20, 15, 40, 255 });
        DrawText("CHILL PLACE", sw/2 - 120, sh/3, 48, { 255, 220, 150, 255 });
    }

    DrawRectangle(0, (int)(sh * 0.65f), sw, (int)(sh * 0.35f),
                  { 0, 0, 0, (unsigned char)(150 * fadeAlpha) });

    float pscale = 1.0f + 0.05f * sinf(pulse);
    const char* label = "[ JOUER ]";
    int fontSize = (int)(36 * pscale);
    if (fontSize < 16) fontSize = 16;
    int tw = MeasureText(label, fontSize);
    Color btnColor = { 255, 230, 100, (unsigned char)(220 * fadeAlpha) };
    DrawText(label, sw/2 - tw/2, (int)(sh * 0.78f), fontSize, btnColor);

    const char* sub = "Appuie n'importe ou en bas pour commencer";
    int stw = MeasureText(sub, 18);
    DrawText(sub, sw/2 - stw/2, (int)(sh * 0.89f), 18,
             { 200, 200, 200, (unsigned char)(180 * fadeAlpha) });
}

void Menu::Unload() {
    if (loaded && texTitle.id > 0) {
        UnloadTexture(texTitle);
        texTitle = Texture2D{};
    }
    loaded = false;
}
