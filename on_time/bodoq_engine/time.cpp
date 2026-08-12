#include "time.h"
#include "win32.h"

double time::ms()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    double freq = ((double)li.QuadPart) / 1000.0;
    QueryPerformanceCounter(&li);
    return (double)(((double)li.QuadPart) / freq);
}
