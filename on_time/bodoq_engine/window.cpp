#include "win32.h"
#include "window.h"
#include "math.h"
#include "input.h"
#include "debug.h"
#include "hash_table.h"

#include <commdlg.h>

extern bool mouseBtnsDown[3];
extern bool mouseBtnsClick[3];

static THashTable<HWND, Window*> internalWindowHandleTable(10);

Window* win::createWindow(const char* title, int32_t x, int32_t y, int32_t width, int32_t height, bool fullscreen, bool allowResize)
{
    if (fullscreen) 
    {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    /* Create Window */
    WNDCLASS windowClass = {
        0,
        [](HWND windowHandle, UINT message, WPARAM wParam,
                          LPARAM lParam) -> LRESULT {
            switch (message) 
            {
            case WM_SIZE:
                if (Window** windowPtr = internalWindowHandleTable.find(windowHandle))
                {
                    uint32_t newWidth = (uint32_t)LOWORD(lParam);
                    uint32_t newHeight = (uint32_t)HIWORD(lParam);
                    if (newWidth > 0 && newHeight > 0 && *windowPtr)
                    {
                        (*windowPtr)->resizeUint[0] = LOWORD(lParam);
                        (*windowPtr)->resizeUint[1] = HIWORD(lParam);
                    }
                }
                break;
            case WM_CHAR:
                input::setLastChar((char)wParam);
                break;
            case WM_EXITSIZEMOVE:
                mouseBtnsDown[0] = false;
                mouseBtnsClick[0] = false;
                break;
            case WM_CLOSE:
                DestroyWindow(windowHandle);
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            case WM_NCLBUTTONUP:
            case WM_CONTEXTMENU:
            case WM_ENTERSIZEMOVE:
            default:
                return DefWindowProc(windowHandle, message, wParam, lParam);
            }
            return S_OK;
        },
        0,
        0,
        GetModuleHandle(nullptr),
        LoadIcon(nullptr, IDI_APPLICATION),
        LoadCursor(nullptr, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        nullptr,
        L"WindowClass"
    };
    RegisterClass(&windowClass);

    DWORD windowStyle = WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_BORDER;
    if (allowResize) 
    {
        windowStyle |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    }
    if (fullscreen) windowStyle = WS_POPUP | WS_VISIBLE;

    RECT windowRect = { 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRect(&windowRect, windowStyle, false);
    int32_t adjustedWidth = windowRect.right - windowRect.left;
    int32_t adjustedHeight = windowRect.bottom - windowRect.top;

    HWND windowHandle = CreateWindowExA(
        WS_EX_APPWINDOW, "WindowClass", title, windowStyle, x,
        y, adjustedWidth, adjustedHeight, nullptr, nullptr,
        GetModuleHandle(nullptr), nullptr);

    if (!fullscreen)
    {
        RECT rc;
        GetWindowRect(windowHandle, &rc);

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        int windowWidth = rc.right - rc.left;
        int windowHeight = rc.bottom - rc.top;

        int posX = (screenWidth - windowWidth) / 2;
        int posY = (screenHeight - windowHeight) / 2;

        SetWindowPos(windowHandle, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        ShowWindow(windowHandle, SW_SHOW);
        UpdateWindow(windowHandle);
    }

    if (!windowHandle)
    {
        DBG_PANIC("Failed to create window");
        return nullptr;
    }

	Window* window = new Window();
    window->windowHandle = windowHandle;
	window->size = Float2((float)width, (float)height);
    window->sizeUint[0] = (uint32_t)width;
    window->sizeUint[1] = (uint32_t)height;
	window->aspectRatio = (float)width / (float)height;
    window->resizeUint[0] = (uint32_t)width;
    window->resizeUint[1] = (uint32_t)height;

    internalWindowHandleTable.insert(windowHandle, window);

    return window;
}

void win::destroyWindow(Window* window)
{
    if (window)
    {
        internalWindowHandleTable.remove((HWND)window->windowHandle);
        DestroyWindow((HWND)window->windowHandle);
        delete window;
	}
}

bool win::dialogOpen(Window* window, const wchar_t* filter, const wchar_t* extensions, WString& outPath)
{
    wchar_t fileBuf[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)window->windowHandle;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = extensions;

    if (GetOpenFileNameW(&ofn))
    {
        outPath = fileBuf;
        return true;
    }
    return false;
}

bool win::dialogSave(Window* window, const wchar_t* filter, const WString& suggestedName, const wchar_t* extensions, WString& outPath)
{
    wchar_t fileBuf[MAX_PATH] = L"";
    memcpy(fileBuf, *suggestedName, suggestedName.byteLength());

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)window->windowHandle;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = extensions;

    if (GetSaveFileNameW(&ofn))
    {
        outPath = fileBuf;
        return true;
    }
    return false;
}

bool win::shouldResize(Window* window)
{
    return window && (window->resizeUint[0] != window->sizeUint[0] || window->resizeUint[1] != window->sizeUint[1]);
}
