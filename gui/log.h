#pragma once
#include "types.h"

extern char logBuf[LOG_LINES][128];
extern int  logCount;

void addLog(const char *fmt, ...);
