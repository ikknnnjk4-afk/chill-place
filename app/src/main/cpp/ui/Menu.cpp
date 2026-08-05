#include "Menu.h"
#include "raylib.h"
#include <cmath>

void Menu::Load() {
    texTitle = LoadTexture("ui/titre_ecran_menu_principal.png");
    SetTextureFilter(texTitle, TEXTURE_FILTER_BILINEAR);
    loaded   = true;
    fadeAlpha = 0.0f;
    pulse     = 0.0f;
}

bool Menu::Update() {
    float dt = GetFrameTime();

    // Fade in
    if (fadeAlpha < 1.0f) {
        fadeAlpha += dt * 0.8f;
        if (fadeAlpha > 1.0f) fadeAlpha = 1.0f;
    }

    pulse += dt * 2.0f;

    // Detect tap / click anywhere on lower half of screen
    bool tapped = false;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 pos = GetMousePosition();
        if (pos.y > GetScreenHeight() * 0.6f) tapped = true;
    }
    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 tp = GetTouchPosition(i);
        if (tp.y > GetScreenHeight() * 0.6f) tapped = true;
    }

    return tapped && fadeAlpha >= 0.9f;
}

void Menu::Draw() const {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Draw title image as full-screen background
    if (loaded && texTitle.id > 0) {
        float scaleX = (float)sw / texTitle.width;
        float scaleY = (float)sh / texTitle.height;
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
        // Fallback gradient if texture failed
        DrawRectangle(0, 0, sw, sh, { 20, 15, 40, 255 });
        DrawText("CHILL PLACE", sw/2 - 120, sh/3, 48, { 255, 220, 150, 255 });
    }

    // Dark overlay on lower portion for readability
    DrawRectangle(0, (int)(sh * 0.65f), sw, (int)(sh * 0.35f),
                  { 0, 0, 0, (unsigned char)(150 * fadeAlpha) });

    // Pulsing "JOUER" button
    float pscale = 1.0f + 0.05f * sinf(pulse);
    const char* label = "[ JOUER ]";
    int fontSize = (int)(36 * pscale);
    int tw = MeasureText(label, fontSize);
    Color btnColor = { 255, 230, 100, (unsigned char)(220 * fadeAlpha) };
    DrawText(label, sw/2 - tw/2, (int)(sh * 0.78f), fontSize, btnColor);

    // Subtitle
    const char* sub = "Appuie n'importe où en bas pour commencer";
    int stw = MeasureText(sub, 18);
    DrawText(sub, sw/2 - stw/2, (int)(sh * 0.89f), 18,
             { 200, 200, 200, (unsigned char)(180 * fadeAlpha) });
}

void Menu::Unload() {
    if (loaded) {
        UnloadTexture(texTitle);
        loaded = false;
    }
}
