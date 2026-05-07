#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "log.h"

char logBuf[LOG_LINES][128];
int  logCount = 0;

void addLog(const char *fmt, ...) {
    if (logCount >= LOG_LINES) {
        memmove(logBuf[0], logBuf[1], (LOG_LINES - 1) * sizeof(logBuf[0]));
        logCount = LOG_LINES - 1;
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logBuf[logCount++], sizeof(logBuf[0]), fmt, ap);
    va_end(ap);
}
