#pragma once
#include <string>

// Call once at the very start of main(), before InitWindow.
void CrashReporter_Install();

// Append a breadcrumb line (also goes to logcat).
void CrashReporter_Log(const char* msg);

// If a previous crash file exists, return its contents (empty if none).
std::string CrashReporter_LoadLastCrash();

// Delete the crash file after the user has seen it.
void CrashReporter_Clear();

// Path helpers (for UI message)
const char* CrashReporter_CrashFilePath();
