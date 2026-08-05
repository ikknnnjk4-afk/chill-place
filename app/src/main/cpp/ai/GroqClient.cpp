#include "GroqClient.h"
#include "raylib.h"
#include <thread>
#include <mutex>
#include <android/log.h>
#include <jni.h>

#define LOG_TAG "GroqClient"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── JVM + ClassLoader (populated by JNI_OnLoad on the main Java thread) ─────
// Fix: FindClass() called from a native thread attached with AttachCurrentThread
// uses the *system* ClassLoader and cannot find application classes like
// com/chillplace/game/GroqBridge. We cache the app ClassLoader here so it is
// always available from any thread.
JavaVM*   gJavaVM      = nullptr;
jobject   gClassLoader = nullptr;
jmethodID gFindClass   = nullptr;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    gJavaVM = vm;

    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnLoad: GetEnv failed");
        return JNI_ERR;
    }

    // Grab the app ClassLoader via any application class known at this point.
    // GroqBridge lives in the same package, so we use it as the anchor.
    jclass anchorClass = env->FindClass("com/chillplace/game/GroqBridge");
    if (!anchorClass) {
        // Clear the pending exception so we don't crash JNI_OnLoad itself.
        env->ExceptionClear();
        LOGE("JNI_OnLoad: anchor class not found – ClassLoader not cached");
        LOGI("JNI_OnLoad: JavaVM stored (no ClassLoader cache)");
        return JNI_VERSION_1_6;
    }

    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID getClassLoader = env->GetMethodID(
        classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject classLoader = env->CallObjectMethod(anchorClass, getClassLoader);

    // Store a global reference so it survives beyond JNI_OnLoad
    gClassLoader = env->NewGlobalRef(classLoader);

    jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
    gFindClass = env->GetMethodID(
        classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    env->DeleteLocalRef(anchorClass);
    env->DeleteLocalRef(classClass);
    env->DeleteLocalRef(classLoader);
    env->DeleteLocalRef(classLoaderClass);

    LOGI("JNI_OnLoad: JavaVM + ClassLoader cached successfully");
    return JNI_VERSION_1_6;
}

// Thread-safe FindClass that works from any native thread
static jclass SafeFindClass(JNIEnv* env, const char* name) {
    if (gClassLoader && gFindClass) {
        jstring jName = env->NewStringUTF(name);
        auto cls = reinterpret_cast<jclass>(
            env->CallObjectMethod(gClassLoader, gFindClass, jName));
        env->DeleteLocalRef(jName);
        return cls;
    }
    // Fallback (only works from the main Java thread)
    return env->FindClass(name);
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

    // Use SafeFindClass instead of env->FindClass so this works from any thread
    jclass cls = SafeFindClass(env, "com/chillplace/game/GroqBridge");
    if (!cls || env->ExceptionCheck()) {
        env->ExceptionClear();
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

            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                result = "ERREUR: Exception Java";
            } else if (jResult) {
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
    history.push_back({ "user", message });

    std::thread([this, message, onResponse]() {
        std::string reply = SendBlocking(message);
        history.push_back({ "assistant", reply });

        {
            std::lock_guard<std::mutex> lock(gPendingMutex);
            gPending.text     = reply;
            gPending.callback = onResponse;
            gPending.ready    = true;
        }

        busy = false;
    }).detach();
}

extern "C" void GroqClient_PollPending() {
    std::lock_guard<std::mutex> lock(gPendingMutex);
    if (gPending.ready && gPending.callback) {
        gPending.callback(gPending.text);
        gPending.callback = nullptr;
        gPending.ready    = false;
    }
}
