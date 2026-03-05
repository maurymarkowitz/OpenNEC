/* Minimal portability compat header for Windows shims
 * This header is intended to be included from central headers (e.g. misc.h)
 * and provides a `clock_gettime` declaration and a few string mappings
 * when compiling on Windows. It is intentionally minimal; implementations
 * live in corresponding .c files when necessary.
 */

#ifndef OPENNEC_COMPAT_H
#define OPENNEC_COMPAT_H

#if defined(_WIN32)

#include <stdint.h>
#include <time.h>
#include <string.h>

/* Define struct timespec for Windows if missing */
#ifndef _TIMESPEC_DEFINED
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
#define _TIMESPEC_DEFINED
#endif

/* Provide clock ids used by the code */
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif

/* Declare shim implementation in compat_time.c */
int clock_gettime(int clk_id, struct timespec *tp);

/* String function mappings for MSVC/Windows CRT */
#if defined(_MSC_VER)
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

/* portable restrict macro */
#ifndef restrict
#if defined(_MSC_VER)
#define restrict __restrict
#else
#define restrict
#endif
#endif

/* noop attribute for non-GNU compilers */
#ifndef ATTRIBUTE_PRINTF
#if defined(__GNUC__)
#define ATTRIBUTE_PRINTF(a,b) __attribute__((format(printf,a,b)))
#else
#define ATTRIBUTE_PRINTF(a,b)
#endif
#endif

#else /* non-Windows */

#include <time.h>

/* Ensure ATTRIBUTE_PRINTF is always available */
#ifndef ATTRIBUTE_PRINTF
#if defined(__GNUC__)
#define ATTRIBUTE_PRINTF(a,b) __attribute__((format(printf,a,b)))
#else
#define ATTRIBUTE_PRINTF(a,b)
#endif
#endif

#endif /* _WIN32 */

/* Provide strcasestr on platforms that don't have it (Windows CRT lacks it) */
#if defined(_WIN32)
static char *strcasestr(const char *haystack, const char *needle)
{
    if (!haystack || !needle) return NULL;
    size_t nlen = strlen(needle);
    if (nlen == 0) return (char *)haystack;
    for (const char *p = haystack; *p; p++) {
        if (strncasecmp(p, needle, (int)nlen) == 0)
            return (char *)p;
    }
    return NULL;
}
#endif

#endif /* OPENNEC_COMPAT_H */
