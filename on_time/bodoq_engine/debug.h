#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#define DBG_EXIT(code) { ExitProcess(code); }
#define DBG_DEBUG_BREAK() __debugbreak()
#define DBG_LOG(fmt, ...) dbg::logFmt(fmt "\n", ##__VA_ARGS__)
#define DBG_PANIC(fmt, ...) { dbg::logFmt("PANIC: " fmt "\n", ##__VA_ARGS__); dbg::processCrashCallback(); DBG_DEBUG_BREAK(); DBG_EXIT(~0); }
#define DBG_ASSERT(x, fmt, ...) if (!(x)) { dbg::logFmt("ASSERT: " fmt "\n", ##__VA_ARGS__); DBG_DEBUG_BREAK(); }

namespace dbg
{
	typedef void(*CrashCallback)(void* userData);

	void processCrashCallback();
	void logFmt(const char* fmt, ...);
	void onCrash(void* userData, CrashCallback callback);
}

