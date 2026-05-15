#pragma once
#include <stdbool.h>

extern bool connected;
extern char serialPort[64];

/* Opens the serial port from argv if on Linux; returns true if connected. */
bool serialInit(int argc, char *argv[]);

/* Sends a NUL-terminated string to the ESP32 (no-op on non-Linux). */
void serialSend(const char *msg);

/* Reads any available bytes from the serial port and dispatches messages. */
void serialPollRead(void);

/* Closes the serial file descriptor. */
void serialClose(void);
