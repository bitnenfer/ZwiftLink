#include "pix.h"
#include "win32.h"
#include "debug.h"
#include "string.h"
#include <strsafe.h>
#include <string>
#include <shlobj.h>
#if PIX_ENABLE_DEBUG_API
#include <pix3.h>
#endif
#include <stdarg.h>

static bool isPixLoaded = false;
static WString pixCaptureName{};

bool pix::loadPIX()
{
#if PIX_ENABLE_DEBUG_API
    if (isPixLoaded)
    {
        DBG_LOG("PIX already loaded");
        return true;
    }
    if (GetModuleHandleA("WinPixGpuCapture.dll") == 0) {

        LPWSTR programFilesPath = nullptr;
        SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL,
            &programFilesPath);

        WString pixSearchPath = programFilesPath + WString(L"\\Microsoft PIX\\*");

        WIN32_FIND_DATAW findData;
        bool foundPixInstallation = false;
        wchar_t newestVersionFound[MAX_PATH];

        HANDLE hFind = FindFirstFileW(pixSearchPath.rawStr(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
                    FILE_ATTRIBUTE_DIRECTORY) &&
                    (findData.cFileName[0] != '.')) {
                    if (!foundPixInstallation ||
                        wcscmp(newestVersionFound, findData.cFileName) <= 0) {
                        foundPixInstallation = true;
                        StringCchCopyW(newestVersionFound,
                            _countof(newestVersionFound),
                            findData.cFileName);
                    }
                }
            } while (FindNextFileW(hFind, &findData) != 0);
        }

        FindClose(hFind);

        if (foundPixInstallation) {
            wchar_t output[MAX_PATH];
            StringCchCopyW(output, pixSearchPath.length(), *pixSearchPath);
            StringCchCatW(output, MAX_PATH, &newestVersionFound[0]);
            StringCchCatW(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");
            LoadLibraryW(output);
            isPixLoaded = true;
            PIXSetHUDOptions(PIX_HUD_SHOW_ON_NO_WINDOWS);
            DBG_LOG("PIX is loaded");
            return true;
        }
    }
    isPixLoaded = false;
#endif
    return false;
}

void pix::beginEvent(ID3D12GraphicsCommandList* ctx, uint64_t color, const char* fmt, ...)
{
#if PIX_ENABLE_DEBUG_API
	if (!isPixLoaded) return;
    char buffer[128] = {};
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, 128, fmt, args);
    va_end(args);
    PIXBeginEvent(ctx, color, buffer);
#endif
}

void pix::endEvent(ID3D12GraphicsCommandList* ctx)
{
#if PIX_ENABLE_DEBUG_API
    if (!isPixLoaded) return;
    PIXEndEvent(ctx);
#endif
}

void pix::beginEvent(ID3D12CommandQueue* ctx, uint64_t color, const char* fmt, ...)
{
#if PIX_ENABLE_DEBUG_API
    if (!isPixLoaded) return;
    char buffer[128] = {};
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, 128, fmt, args);
    va_end(args);
    PIXBeginEvent(ctx, color, buffer);
#endif
}

void pix::endEvent(ID3D12CommandQueue* ctx)
{
#if PIX_ENABLE_DEBUG_API
    if (!isPixLoaded) return;
    PIXEndEvent(ctx);
#endif
}

bool pix::beginCapture(const char* captureName)
{
#if PIX_ENABLE_DEBUG_API
    if (!isPixLoaded) return false;
    wchar_t dateStr[64]{};
    SYSTEMTIME st{};
    GetLocalTime(&st);
    GetDateFormatW(LOCALE_CUSTOM_DEFAULT, DATE_AUTOLAYOUT, &st, L"dd-MM-yyy", dateStr, 64);
    WString name = str::format(L"%s_capture_%s-%02d-%02d-%02d.wpix", 
        *str::toStringW(captureName), 
        dateStr, 
        st.wHour, 
        st.wMinute, 
		st.wSecond);
    PIXCaptureParameters params{};
    params.GpuCaptureParameters.FileName = *name;
    pixCaptureName = name;
    return PIXBeginCapture(PIX_CAPTURE_GPU, &params) == S_OK;
#else
    return false;
#endif
}

bool pix::endCapture(bool openCapture)
{
#if PIX_ENABLE_DEBUG_API
    if (!isPixLoaded) return false;
    if (PIXEndCapture(FALSE) == S_OK)
    {
        if (openCapture)
        {
            PIXOpenCaptureInUI(*pixCaptureName);
		}
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool pix::isPixEnabled()
{
#if PIX_ENABLE_DEBUG_API
    return isPixLoaded;
#else
	return false;
#endif
}
