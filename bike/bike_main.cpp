#include "BikeTrainerLink.h"

#include <Windows.h>
#include <stdio.h>

#define DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE  (4096 * 1024)
#define DBG_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT 4

static void logFmt(const char* fmt, ...) {
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
#define DBG_LOG(fmt, ...) logFmt(fmt "\n", ##__VA_ARGS__)

int main()
{
    const char* ButtonNames[] =
    {
        "LEFT",
        "UP",
        "RIGHT",
        "DOWN",
        "MINUS",
        "A",
        "B",
        "Y",
        "Z",
        "PLUS"
    };

    int32_t connectionMsgCheck = 0;
    FBikeLink bikeLink{};
    while (true)
    {
        bikeLink.Update();

        if (bikeLink.IsConnectionInProgress() && connectionMsgCheck == 0)
        {
            DBG_LOG("Connecting...");
            connectionMsgCheck++;
            continue;
        }
        else if (!bikeLink.IsConnectionInProgress() && connectionMsgCheck > 0)
        {
            connectionMsgCheck = 0;
            if (bikeLink.IsConnected())
            {
                DBG_LOG("Bike connected.");
            }
            else
            {
                DBG_LOG("Bike NOT connected.");
            }
        }

        for (uint8_t index = 0; index < (uint8_t)EZwiftClickButton::COUNT; ++index)
        {
            if (bikeLink.WasPressed((EZwiftClickButton)index))
                DBG_LOG("Pressed %s", ButtonNames[index]);
        }
    }

    return 0;
}
