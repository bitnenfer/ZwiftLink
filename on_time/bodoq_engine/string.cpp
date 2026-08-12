#include "string.h"
#include <stdarg.h>
#include <stdio.h>

String str::format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
	String result = str::formatv(fmt, args);
	va_end(args);
	return result;
}

String str::formatv(const char* fmt, va_list args)
{
	int required = _vscprintf(fmt, args) + 1;
	char* buffer = new char[required];
	vsprintf_s(buffer, required, fmt, args);
	String output = String(buffer);
	delete[] buffer;
	return output;
}

WString str::format(const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WString result = str::formatv(fmt, args);
	va_end(args);
    return result;
}

WString str::formatv(const wchar_t* fmt, va_list args)
{
	wchar_t buffer[4096];
	vswprintf_s(buffer, 4096, fmt, args);
	return WString(buffer);
}
