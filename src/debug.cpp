#include "../rtt/SEGGER_RTT.h"
#include "stdio.h"
#include "debug.h"
#include <stdarg.h>
#include "string.h"

// this is a make-shift version of RTT_vprintf, but uses Arduino printf to reduce executable size.
// the maximum size which can be printed in one call of the function, is up to DBG_PRINTF_MAX_CHARS.


int __dbg_printf(const char * sFormat, ...)
{
    int result;
    va_list args;
    char buf[DBG_PRINTF_MAX_CHARS+1];

    va_start(args, sFormat);

    result = vsnprintf(buf, sizeof(buf-1), sFormat, args);

    va_end(args);

    buf[sizeof(buf)-1] = 0; // terminate the string anyway

    SEGGER_RTT_Write(0, buf, strlen(buf));

    return result;
}
