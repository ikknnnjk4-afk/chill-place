#include "GroqClient.h"
#include "raylib.h"
#include <thread>
#include <mutex>
#include <android/log.h>
#include <jni.h>

#define LOG_TAG "GroqClient"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── JVM access (populated by JNI_OnLoad) — non-static for extern access ─────
JavaVM* gJavaVM = nullptr;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    gJavaVM = vm;
    LOGI("JNI_OnLoad: JavaVM stored");
    return JNI_VERSION_1_6;
}

// ── Mutex for pending responses ──────────────────────────────────────────────
struct PendingResponse {
    std::string text;
    std::function<void(const std::string&)> callback;
    bool ready = false;
};

static std::mutex               gPendingMutex;
static PendingResponse          gPending;

// ── GroqClient ───────────────────────────────────────────────────────────────

GroqClient::GroqClient(const std::string& key) : apiKey(key) {}

void GroqClient::ResetHistory() {
    history.clear();
}

std::string GroqClient::HistoryToJson() const {
    if (history.empty()) return "";
    std::string json = "[";
    for (size_t i = 0; i < history.size(); ++i) {
        json += "{\"role\":\"" + history[i].role + "\",\"content\":\"";
        // Escape quotes
        for (char c : history[i].content) {
            if (c == '"') json += "\\\"";
            else if (c == '\n') json += "\\n";
            else json += c;
        }
        json += "\"}";
        if (i + 1 < history.size()) json += ",";
    }
    json += "]";
    return json;
}

std::string GroqClient::SendBlocking(const std::string& message) {
    if (!gJavaVM) {
        LOGE("No JavaVM available");
        return "ERREUR: Pas de JVM";
    }

    JNIEnv* env = nullptr;
    bool attached = false;
    jint rc = gJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (rc == JNI_EDETACHED) {
        gJavaVM->AttachCurrentThread(&env, nullptr);
        attached = true;
    } else if (rc != JNI_OK || !env) {
        LOGE("Cannot get JNIEnv");
        return "ERREUR: JNIEnv indisponible";
    }

    std::string result;

    jclass cls = env->FindClass("com/chillplace/game/GroqBridge");
    if (!cls) {
        LOGE("GroqBridge class not found");
        result = "ERREUR: Classe introuvable";
    } else {
        jmethodID mid = env->GetStaticMethodID(cls, "sendMessage",
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");

        if (!mid) {
            LOGE("sendMessage method not found");
            result = "ERREUR: Methode introuvable";
        } else {
            jstring jKey  = env->NewStringUTF(apiKey.c_str());
            jstring jMsg  = env->NewStringUTF(message.c_str());
            jstring jHist = env->NewStringUTF(HistoryToJson().c_str());

            jstring jResult = reinterpret_cast<jstring>(
                env->CallStaticObjectMethod(cls, mid, jKey, jMsg, jHist));

            if (jResult) {
                const char* cStr = env->GetStringUTFChars(jResult, nullptr);
                result = cStr;
                env->ReleaseStringUTFChars(jResult, cStr);
                env->DeleteLocalRef(jResult);
            } else {
                result = "ERREUR: Pas de réponse";
            }

            env->DeleteLocalRef(jKey);
            env->DeleteLocalRef(jMsg);
            env->DeleteLocalRef(jHist);
        }
        env->DeleteLocalRef(cls);
    }

    if (attached) gJavaVM->DetachCurrentThread();
    return result;
}

void GroqClient::SendAsync(const std::string& message,
                            std::function<void(const std::string&)> onResponse)
{
    if (busy) {
        onResponse("Je réfléchis encore… patiente un instant !");
        return;
    }

    busy = true;

    // Record the user message in history
    history.push_back({ "user", message });

    // Run in a background thread to avoid blocking the game loop
    std::thread([this, message, onResponse]() {
        std::string reply = SendBlocking(message);

        // Record AI reply in history
        history.push_back({ "assistant", reply });

        // Queue the response for main-thread delivery
        {
            std::lock_guard<std::mutex> lock(gPendingMutex);
            gPending.text     = reply;
            gPending.callback = onResponse;
            gPending.ready    = true;
        }

        busy = false;
    }).detach();
}

// Called from HUD each frame to deliver queued responses on the main thread
extern "C" void GroqClient_PollPending() {
    std::lock_guard<std::mutex> lock(gPendingMutex);
    if (gPending.ready && gPending.callback) {
        gPending.callback(gPending.text);
        gPending.callback = nullptr;
        gPending.ready    = false;
    }
}
