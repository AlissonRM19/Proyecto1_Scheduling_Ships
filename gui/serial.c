#include <string.h>
#include "serial.h"
#include "ship_pool.h"
#include "log.h"
#include "types.h"

bool connected  = false;
char serialPort[64] = "/dev/ttyUSB0";

#ifdef __linux__
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

static int  serialFd    = -1;
static char serialRxBuf[512];
static int  serialRxLen = 0;

/* ── Configures the serial port at 115200 8N1 non-blocking ───────────── */
static bool serialOpen(const char *port) {
    serialFd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serialFd < 0) return false;
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag = CS8 | CLOCAL | CREAD;
    tty.c_iflag = IGNPAR;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tcflush(serialFd, TCIFLUSH);
    tcsetattr(serialFd, TCSANOW, &tty);
    return true;
}

/* ── Parses one "GUI:..." line sent by the ESP32 ─────────────────────── */
static void parseGUILine(const char *line) {
    if (strncmp(line, "GUI:", 4) != 0) return;
    const char *msg = line + 4;

    int id, tipo, lado, prioridad, tiempoRestante, deadline;
    int sliceActual, tiempoCruce, num;

    if (sscanf(msg, "ADD,%d,%d,%d,%d,%d,%d",
               &id, &tipo, &lado, &prioridad, &tiempoRestante, &deadline) == 6) {
        Barco *b = allocBarco();
        if (!b) return;
        memset(b, 0, sizeof(Barco));
        b->id = id; b->tipo = tipo; b->lado = lado;
        b->prioridad = prioridad; b->tiempoRestante = tiempoRestante;
        b->tiempoCruce = tiempoRestante; b->deadline = deadline;
        b->activo = true; b->estado = EST_ESPERANDO;
        Cola *c = (lado == 0) ? &colaIzq : &colaDer;
        colaEnqueue(c, b);
        addLog("ESP32: Barco %d (%s, %s)", id, strTipo(tipo), lado == 0 ? "Izq" : "Der");

    } else if (sscanf(msg, "CROSS,%d,%d,%d,%d,%d,%d",
               &id, &tipo, &lado, &sliceActual, &tiempoCruce, &tiempoRestante) == 6) {
        Barco *b = findBarcoById(id);
        if (!b) {
            b = allocBarco(); if (!b) return;
            memset(b, 0, sizeof(Barco));
            b->id = id; b->tipo = tipo; b->lado = lado; b->activo = true;
        }
        removeFromQueue(&colaIzq, id);
        removeFromQueue(&colaDer,  id);
        b->tiempoCruce = tiempoCruce; b->tiempoRestante = tiempoRestante;
        b->sliceActual = sliceActual; b->tiempoEnSlice = 0.0f;
        b->estado = EST_CRUZANDO;
        float fracBase = (tiempoCruce > 0)
            ? (float)(tiempoCruce - tiempoRestante) / (float)tiempoCruce : 0.0f;
        float left = (float)CANAL_X, right = (float)(CANAL_X + CANAL_W - SHIP_W);
        b->posX = (lado == 0) ? left + (right - left) * fracBase
                              : right - (right - left) * fracBase;
        b->posY = (float)CANAL_MID_Y;
        b->progreso = fracBase;
        barcoEnCanal = b;
        addLog("ESP32: Barco %d cruza %ds", id, sliceActual);

    } else if (sscanf(msg, "DONE,%d", &id) == 1) {
        if (barcoEnCanal && barcoEnCanal->id == id) {
            barcoEnCanal->estado = EST_TERMINADO;
            barcoEnCanal->activo = false;
            barcoEnCanal = NULL;
        }
        addLog("ESP32: Barco %d termino!", id);

    } else if (sscanf(msg, "RETURN,%d,%d", &id, &tiempoRestante) == 2) {
        Barco *b = barcoEnCanal;
        if (b && b->id == id) {
            b->tiempoRestante = tiempoRestante;
            b->estado = EST_ESPERANDO;
            Cola *c = (b->lado == 0) ? &colaIzq : &colaDer;
            colaEnqueue(c, b);
            barcoEnCanal = NULL;
        }
        addLog("ESP32: Barco %d regresa (resta %ds)", id, tiempoRestante);

    } else if (sscanf(msg, "DIR,%d",  &num) == 1) { sentidoActual = num;
    } else if (sscanf(msg, "SCH,%d",  &num) == 1) { scheduler = (TipoScheduler)num;
    } else if (sscanf(msg, "MODO,%d", &num) == 1) { modo = (ModoFlujo)num;
    }
}

/* ── Public API ──────────────────────────────────────────────────────── */
bool serialInit(int argc, char *argv[]) {
    if (argc > 1) { strncpy(serialPort, argv[1], 63); serialPort[63] = '\0'; }
    return serialOpen(serialPort);
}

void serialSend(const char *msg) {
    if (serialFd >= 0) write(serialFd, msg, strlen(msg));
}

void serialPollRead(void) {
    if (serialFd < 0) return;
    char ch;
    while (read(serialFd, &ch, 1) > 0) {
        if (ch == '\n') {
            serialRxBuf[serialRxLen] = '\0';
            parseGUILine(serialRxBuf);
            serialRxLen = 0;
        } else if (ch != '\r' && serialRxLen < 511) {
            serialRxBuf[serialRxLen++] = ch;
        }
    }
}

void serialClose(void) {
    if (serialFd >= 0) { close(serialFd); serialFd = -1; }
}

#else
/* Stubs for non-Linux builds */
bool serialInit(int argc, char *argv[]) { (void)argc; (void)argv; return false; }
void serialSend(const char *msg)        { (void)msg; }
void serialPollRead(void)               {}
void serialClose(void)                  {}
#endif /* __linux__ */
