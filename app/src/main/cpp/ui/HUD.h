#pragma once
#include "raylib.h"
#include "ai/GroqClient.h"
#include <string>
#include <vector>
#include <memory>

struct ChatLine {
    std::string text;
    bool        isUser;
};

class HUD {
public:
    void Load();
    void Update(float dt);
    void Draw() const;

    // Chat panel
    bool ChatButtonPressed() const;
    void OpenChat();
    void UpdateChat(float dt);
    void DrawChat() const;
    bool ChatClosed() const;

    void Unload();

private:
    // Groq AI client (lazy-init with API key from env)
    std::unique_ptr<GroqClient> groq;
    std::string apiKey;

    // Chat state
    bool chatOpen      = false;
    bool chatCloseReq  = false;
    bool chatButtonTap = false;

    std::vector<ChatLine> chatHistory;
    std::string           inputBuffer;
    float                 chatScrollY = 0.0f;

    std::string aiThinking = "";
    bool        waiting    = false;

    // Virtual joystick (left side)
    Vector2 joyCenter  = {};
    Vector2 joyPos     = {};
    bool    joyActive  = false;
    int     joyTouchId = -1;

    // Look touch (right side)
    Vector2 lookPrev  = {};
    bool    lookActive = false;
    int     lookTouchId = -1;

    void DrawJoystick()    const;
    void DrawChatButton()  const;
    void UpdateJoystick();
    void UpdateLookTouch();

    void SendChatMessage();
    void PollAIResponse();
};
