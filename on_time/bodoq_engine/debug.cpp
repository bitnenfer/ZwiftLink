#include "debug.h"
#include <stdio.h>
#include <stdint.h>

#define DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE  (4096 * 1024)
#define DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT 4

struct CrashCallbackData
{
    dbg::CrashCallback callback;
    void* userData;
};

static CrashCallbackData crashCallbackData{ nullptr, nullptr};

void dbg::logFmt(const char* fmt, ...) {
    static char bufferLarge[DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE * DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT] = {};
    static uint32_t bufferIndex = 0;
    va_list args;
    va_start(args, fmt);
    char* buffer = &bufferLarge[bufferIndex * DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE];
    vsprintf_s(buffer, DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE, fmt, args);
    bufferIndex = (bufferIndex + 1) % DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT;
    va_end(args);
    OutputDebugStringA(buffer);
    printf("%s", buffer);
}

void dbg::processCrashCallback()
{
    if (crashCallbackData.callback)
    {
        crashCallbackData.callback(crashCallbackData.userData);
    }
}

void dbg::onCrash(void* userData, dbg::CrashCallback callback)
{
    crashCallbackData.userData = userData;
    crashCallbackData.callback = callback;
}