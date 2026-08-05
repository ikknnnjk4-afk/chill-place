#include "CrashReporter.h"
#include <android/log.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string>
#include <mutex>

#define LOG_TAG "ChillPlace"
#define CRASH_PATH_INTERNAL "/data/data/com.chillplace.game/files/chillplace_crash.txt"
#define CRASH_PATH_CACHE    "/data/data/com.chillplace.game/cache/chillplace_crash.txt"
#define RING_SIZE 48
#define RING_LINE 192

static char gRing[RING_SIZE][RING_LINE];
static int  gRingIdx = 0;
static std::mutex gRingMutex;
static volatile sig_atomic_t gHandling = 0;

static void EnsureDirs() {
    mkdir("/data/data/com.chillplace.game", 0755);
    mkdir("/data/data/com.chillplace.game/files", 0755);
    mkdir("/data/data/com.chillplace.game/cache", 0755);
}

void CrashReporter_Log(const char* msg) {
    if (!msg) return;
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s", msg);
    std::lock_guard<std::mutex> lock(gRingMutex);
    snprintf(gRing[gRingIdx % RING_SIZE], RING_LINE, "%s", msg);
    gRingIdx++;
}

static void WriteCrashFile(int sig) {
    if (gHandling) return;
    gHandling = 1;

    EnsureDirs();

    const char* path = CRASH_PATH_INTERNAL;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        path = CRASH_PATH_CACHE;
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    if (fd < 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "CRASH signal=%d (no file)", sig);
        _exit(128 + sig);
    }

    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "=== CHILL PLACE CRASH ===\nsignal=%d\npid=%d\n--- breadcrumbs ---\n",
        sig, (int)getpid());
    if (n > 0) write(fd, buf, (size_t)n);

    int base = gRingIdx;
    for (int i = 0; i < RING_SIZE; i++) {
        int idx = (base + i) % RING_SIZE;
        if (gRing[idx][0] == '\0') continue;
        size_t len = strlen(gRing[idx]);
        write(fd, gRing[idx], len);
        write(fd, "\n", 1);
    }
    write(fd, "--- end ---\n", 12);
    fsync(fd);
    close(fd);

    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "CRASH signal=%d -> %s", sig, path);
    _exit(128 + sig);
}

static void SignalHandler(int sig) {
    WriteCrashFile(sig);
}

void CrashReporter_Install() {
    memset(gRing, 0, sizeof(gRing));
    gRingIdx = 0;
    EnsureDirs();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE,  &sa, nullptr);
    sigaction(SIGILL,  &sa, nullptr);
    sigaction(SIGBUS,  &sa, nullptr);

    CrashReporter_Log("CrashReporter installed");
}

std::string CrashReporter_LoadLastCrash() {
    const char* paths[] = { CRASH_PATH_INTERNAL, CRASH_PATH_CACHE };
    for (const char* path : paths) {
        FILE* f = fopen(path, "r");
        if (!f) continue;
        std::string content;
        char buf[512];
        while (fgets(buf, sizeof(buf), f)) content += buf;
        fclose(f);
        if (!content.empty()) return content;
    }
    return {};
}

void CrashReporter_Clear() {
    unlink(CRASH_PATH_INTERNAL);
    unlink(CRASH_PATH_CACHE);
}

const char* CrashReporter_CrashFilePath() {
    return CRASH_PATH_INTERNAL;
}
