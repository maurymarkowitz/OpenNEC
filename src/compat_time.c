/* Windows implementation of clock_gettime shim.
 * Uses QueryPerformanceCounter for high-resolution monotonic time.
 */

#if defined(_WIN32)

#include "compat.h"
#include <windows.h>
#include <stdint.h>

int clock_gettime(int clk_id, struct timespec *tp) {
    (void)clk_id;
    if (!tp) return -1;

    LARGE_INTEGER freq, counter;
    if (!QueryPerformanceFrequency(&freq)) {
        /* Fallback to GetTickCount64 */
        uint64_t ms = GetTickCount64();
        tp->tv_sec = (time_t)(ms / 1000);
        tp->tv_nsec = (long)((ms % 1000) * 1000000);
        return 0;
    }
    QueryPerformanceCounter(&counter);
    double seconds = (double)counter.QuadPart / (double)freq.QuadPart;
    tp->tv_sec = (time_t)seconds;
    tp->tv_nsec = (long)((seconds - (double)tp->tv_sec) * 1e9);
    return 0;
}

/* UCRT 64-bit time variant: simply delegates to our clock_gettime */
int __cdecl clock_gettime64(int clk_id, struct timespec *tp) {
    return clock_gettime(clk_id, tp);
}

#endif
