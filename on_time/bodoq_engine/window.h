#pragma once

#include <stdint.h>
#include "math.h"
#include "string.h"

struct Window
{
	void* windowHandle;
	Float2 size;
	uint32_t sizeUint[2];
	uint32_t resizeUint[2];
	float aspectRatio;
};

namespace win
{
	Window* createWindow(const char* title, int32_t x, int32_t y, int32_t width, int32_t height, bool fullscreen, bool allowResize);
	void destroyWindow(Window* window);
	bool dialogOpen(Window* window, const wchar_t* filter, const wchar_t* extensions, WString& outPath);
	bool dialogSave(Window* window, const wchar_t* filter, const WString& suggestedName, const wchar_t* extensions, WString& outPath);
	bool shouldResize(Window* window);
}
