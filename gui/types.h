#pragma once
#include <stdbool.h>

/* ── Layout ─────────────────────────────────────────────────────────── */
#define SCREEN_W    1280
#define SCREEN_H    720
#define TARGET_FPS  60

#define PANEL_TOP_H 90
#define PANEL_BOT_H 130
#define CANAL_Y     PANEL_TOP_H
#define CANAL_H     (SCREEN_H - PANEL_TOP_H - PANEL_BOT_H)
#define QUEUE_W     255
#define CANAL_X     QUEUE_W
#define CANAL_W     (SCREEN_W - 2 * QUEUE_W)
#define CANAL_MID_Y (CANAL_Y + CANAL_H / 2 - 20)
#define SHIP_W      72
#define SHIP_H      34
#define MAX_QUEUE   10
#define MAX_SHIPS   60
#define LOG_LINES   9

/* ── Enums ───────────────────────────────────────────────────────────── */
typedef enum { MODO_TICO=0, MODO_EQUIDAD=1, MODO_LETRERO=2 } ModoFlujo;

typedef enum {
    SCH_FCFS=1, SCH_PRIORITY=2, SCH_RR=3,
    SCH_SJF=4,  SCH_STRN=5,     SCH_EDF=6
} TipoScheduler;

typedef enum { EST_ESPERANDO=0, EST_CRUZANDO=1, EST_TERMINADO=2 } EstadoBarco;

/* ── Structs ─────────────────────────────────────────────────────────── */
typedef struct {
    int         id;
    int         tipo;
    int         lado;
    int         tiempoCruce;
    int         tiempoRestante;
    int         prioridad;
    int         deadline;
    EstadoBarco estado;
    float       posX, posY;
    float       progreso;
    float       tiempoEnSlice;
    int         sliceActual;
    bool        activo;
} Barco;

typedef struct {
    Barco *items[MAX_QUEUE];
    int    count;
} Cola;
