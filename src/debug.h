#pragma once

#define DBG_PRINTF_MAX_CHARS 128
int __dbg_printf(const char * sFormat, ...);

void dbg_print(const char *s);

#ifdef USE_SEGGER_RTT
#define dbg_printf(...) __dbg_printf(__VA_ARGS__)
#else
#define dbg_printf(...) printf(__VA_ARGS__)
#endif
