#include "file.h"
#include "debug.h"

void* file::readFile(const String& filePath, size_t& outFileSize)
{
    HANDLE fileHandle = CreateFileA(*filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            char* readBuffer = (char*)malloc(fileSize.QuadPart + 1);
            if (!readBuffer)
            {
                CloseHandle(fileHandle);
                DBG_LOG("Failed to allocate read buffer for file %s", *filePath);
                return nullptr;
            }
            memset(readBuffer, 0, fileSize.QuadPart + 1);
            DWORD readBytes = 0;
            if (readBuffer && ReadFile(fileHandle, readBuffer, (DWORD)fileSize.QuadPart, &readBytes, nullptr))
            {
                CloseHandle(fileHandle);
                outFileSize = (size_t)readBytes;
                return readBuffer;
            }
            else
            {
                DBG_LOG("Failed to read file %s", *filePath);
            }
        }
        else
        {
            DBG_LOG("Failed to get file size for %s", *filePath);
        }
        CloseHandle(fileHandle);
    }
    else
    {
        DBG_LOG("Failed to open file %s", *filePath);
    }
    return nullptr;
}

bool file::writeFile(const String& filePath, const void* data, size_t dataSize)
{
    bool result = true;
    HANDLE fileHandle = CreateFileA(*filePath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        if (!WriteFile(fileHandle, data, (DWORD)dataSize, &written, nullptr))
        {
            DBG_LOG("Failed to write to file %s", *filePath);
            result = false;
        }
        CloseHandle(fileHandle);
    }
    else
    {
        DBG_LOG("Failed to open file %s", *filePath);
        result = false;
    }
    return result;
}
