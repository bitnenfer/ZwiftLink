#include "win32.h"
#include "input.h"

static bool keysDown[512];
static bool keysClick[512];
bool mouseBtnsDown[3];
bool mouseBtnsClick[3];
static char lastChar = 0;
static float currMouseX = 0.0f;
static float currMouseY = 0.0f;
static float currMouseWheelY = 0.0f;
static bool shouldQuitApp = false;

void input::pollEvents() {
	static bool firstPoll = true;
    if (firstPoll)
    {
        memset(keysDown, 0, sizeof(keysDown));
        memset(mouseBtnsDown, 0, sizeof(mouseBtnsDown));
        memset(mouseBtnsClick, 0, sizeof(mouseBtnsClick));
        memset(keysClick, 0, sizeof(keysClick));
        firstPoll = false;
    }

    MSG message;
    memset(mouseBtnsClick, 0, sizeof(mouseBtnsClick));
    memset(keysClick, 0, sizeof(keysClick));
    lastChar = 0;
    currMouseWheelY = 0.0f;
    while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);

        switch (message.message)
        {
        case WM_KEYDOWN:
            if (!keysDown[(uint8_t)message.wParam]) 
            {
                keysClick[(uint8_t)message.wParam] = true;
            }
            keysDown[(uint8_t)message.wParam] = true;
            break;
        case WM_KEYUP:
            keysDown[(uint8_t)message.wParam] = false;
            break;
        case WM_MOUSEMOVE:
            currMouseX = (float)GET_X_LPARAM(message.lParam);
            currMouseY = (float)GET_Y_LPARAM(message.lParam);
            break;
        case WM_LBUTTONDOWN:
            if (!mouseBtnsDown[0]) {
                mouseBtnsClick[0] = true;
            }
            mouseBtnsDown[0] = true;
            break;
        case WM_LBUTTONUP:
            mouseBtnsDown[0] = false;
            mouseBtnsClick[0] = false;
            break;
        case WM_NCLBUTTONDOWN:
            mouseBtnsDown[0] = false;
            mouseBtnsClick[0] = false;
            break;
        case WM_RBUTTONDOWN:
            if (!mouseBtnsDown[2]) {
                mouseBtnsClick[2] = true;
            }
            mouseBtnsDown[2] = true;
            break;
        case WM_RBUTTONUP:
            mouseBtnsDown[2] = false;
            mouseBtnsClick[2] = false;
            break;
        case WM_MBUTTONDOWN:
            if (!mouseBtnsDown[1]) {
                mouseBtnsClick[1] = true;
            }
            mouseBtnsDown[1] = true;
            break;
        case WM_MBUTTONUP:
            mouseBtnsDown[1] = false;
            mouseBtnsClick[1] = false;
            break;
        case WM_QUIT:
            shouldQuitApp = true;
            break;
        case WM_MOUSEWHEEL:
            currMouseWheelY = (float)GET_WHEEL_DELTA_WPARAM(message.wParam) / (float)WHEEL_DELTA;
            break;
        default:
            break;
        }
    }
}

float input::mouseX() 
{
    return currMouseX;
}
float input::mouseY() 
{
    return currMouseY;
}

float input::mouseWheelX() 
{
    return 0.0f;
}

float input::mouseWheelY() 
{
    return currMouseWheelY;
}

bool input::mouseDown(MouseButton button) 
{
    return mouseBtnsDown[(uint32_t)button];
}

bool input::mouseClick(MouseButton button) 
{
    return mouseBtnsClick[(uint32_t)button];
}

bool input::keyDown(KeyCode keyCode) 
{
    return keysDown[(uint32_t)keyCode];
}

bool input::keyClick(KeyCode keyCode)
{
    return keysClick[(uint32_t)keyCode];
}

char input::getLastChar() 
{
    return lastChar;
}

bool input::shouldQuit()
{
    return shouldQuitApp;
}

void input::triggerExit()
{
	shouldQuitApp = true;
}

void input::setLastChar(char v)
{
    lastChar = v;
}
