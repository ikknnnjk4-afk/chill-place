#include "Menu.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>

void Menu::Load() {
    texTitle = LoadTexture("ui/titre_ecran_menu_principal.png");
    if (texTitle.id > 0) SetTextureFilter(texTitle, TEXTURE_FILTER_BILINEAR);
    loaded = true;
    phase  = MenuPhase::TITLE;
    timer  = 0.0f;
    fade   = 0.0f;
    pulse  = 0.0f;
}

bool Menu::Update() {
    float dt = GetFrameTime();
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;

    pulse += dt * 2.5f;
    timer += dt;

    switch (phase) {
        case MenuPhase::TITLE:
            // No Play button – wait 5 seconds then auto-transition
            if (timer >= 5.0f) {
                phase = MenuPhase::FADE_OUT;
                timer = 0.0f;
            }
            break;

        case MenuPhase::FADE_OUT:
            fade += dt * 1.2f;
            if (fade >= 1.0f) {
                fade  = 1.0f;
                phase = MenuPhase::CONTROLLER_SCAN;
                timer = 0.0f;
            }
            break;

        case MenuPhase::CONTROLLER_SCAN:
            scanDots = ((int)(timer * 2.0f)) % 4;
            // Scan for ~3.5s then enter game
            if (timer >= 3.5f) {
                phase = MenuPhase::DONE;
            }
            break;

        case MenuPhase::DONE:
            return true;
    }
    return false;
}

void Menu::Draw() const {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (sw <= 0 || sh <= 0) return;

    if (phase == MenuPhase::TITLE || phase == MenuPhase::FADE_OUT) {
        // Background title image
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
                { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangle(0, 0, sw, sh, { 25, 15, 45, 255 });
        }

        // Retro blinking title
        float blink = 0.55f + 0.45f * sinf(pulse * 1.8f);
        Color col = {
            (unsigned char)(180 + 75 * blink),
            (unsigned char)(80 + 40 * blink),
            (unsigned char)(255 * blink),
            255
        };
        const char* title = "CHILL PLACE";
        int fs = 48;
        int tw = MeasureText(title, fs);
        DrawText(title, sw/2 - tw/2, sh/5, fs, col);

        // Subtle cyan outline
        Color cyan = { 80, 255, 255, (unsigned char)(120 * blink) };
        DrawText(title, sw/2 - tw/2 - 2, sh/5 - 2, fs, cyan);

        // Hint: no button
        DrawText("...", sw/2 - 10, sh - 60, 24, { 200, 200, 255, 150 });
    }

    if (phase == MenuPhase::FADE_OUT || phase == MenuPhase::CONTROLLER_SCAN) {
        unsigned char a = (unsigned char)(fade * 255);
        if (phase == MenuPhase::CONTROLLER_SCAN) a = 255;
        DrawRectangle(0, 0, sw, sh, { 0, 0, 0, a });
    }

    if (phase == MenuPhase::CONTROLLER_SCAN) {
        const char* msg = "Recherche des Joy-Con et des manettes";
        int fs = 28;
        int tw = MeasureText(msg, fs);
        DrawText(msg, sw/2 - tw/2, sh/2 - 40, fs, { 200, 220, 255, 255 });

        char dots[8] = {};
        for (int i = 0; i < scanDots && i < 3; i++) dots[i] = '.';
        int dw = MeasureText(dots, 40);
        DrawText(dots, sw/2 - dw/2, sh/2 + 10, 40, { 255, 230, 100, 255 });

        DrawText("Joy-Con  |  Switch Pro  |  Xbox  |  PS",
                 sw/2 - MeasureText("Joy-Con  |  Switch Pro  |  Xbox  |  PS", 18)/2,
                 sh/2 + 70, 18, { 150, 150, 180, 200 });
    }
}

void Menu::Unload() {
    if (texTitle.id > 0) UnloadTexture(texTitle);
    texTitle = {};
    loaded = false;
}
