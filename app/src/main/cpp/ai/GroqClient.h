#pragma once
#include <string>
#include <vector>
#include <functional>

struct ChatMessage {
    std::string role;    // "user" or "assistant"
    std::string content;
};

class GroqClient {
public:
    // apiKey: Groq API key (embedded via BuildConfig)
    explicit GroqClient(const std::string& apiKey);

    // Send a message asynchronously (non-blocking on Android via thread)
    // onResponse is called on the main thread with the response text
    void SendAsync(const std::string& message,
                   std::function<void(const std::string&)> onResponse);

    // Clear conversation history
    void ResetHistory();

    bool IsBusy() const { return busy; }

private:
    std::string              apiKey;
    std::vector<ChatMessage> history;
    bool                     busy = false;

    // Blocking send (runs on worker thread)
    std::string SendBlocking(const std::string& message);

    // Build JSON history string
    std::string HistoryToJson() const;
};
