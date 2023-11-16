#ifdef USE_SEGGER_RTT
    #include "../rtt/SEGGER_RTT.h"
#endif
#include "stdio.h"
#include "debug.h"
#include <stdarg.h>
#include "string.h"

// this is a make-shift version of RTT_vprintf, but uses Arduino printf to reduce executable size.
// the maximum size which can be printed in one call of the function, is up to DBG_PRINTF_MAX_CHARS.

#ifdef USE_SEGGER_RTT
int __dbg_printf(const char * sFormat, ...)
{
    int result;
    va_list args;
    char buf[DBG_PRINTF_MAX_CHARS];

    va_start(args, sFormat);

    result = vsnprintf(buf, sizeof(buf), sFormat, args);

    va_end(args);

    SEGGER_RTT_Write(0, buf, strlen(buf));

    return result;
}

void dbg_print(const char *s)
{
    SEGGER_RTT_Write(0, s, strlen(s));
}

#else

void dbg print(const char *s)
{
    printf("%s", s);
}
#endif