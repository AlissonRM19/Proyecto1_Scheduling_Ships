/*
 * simulacion_gui.c — Simulacion grafica del Canal de Barcos
 *
 * Reimplementa la logica del ESP32/FreeRTOS como una simulacion en PC
 * usando raylib para graficos 2D.
 *
 * Controles:
 *   1/2/3 -> Agregar barco Normal / Pesquero / Patrulla al lado IZQUIERDO
 *   4/5/6 -> Agregar barco Normal / Pesquero / Patrulla al lado DERECHO
 *   S     -> Ciclar scheduler (FCFS -> PRIORITY -> RR -> SJF -> STRN -> EDF)
 *   M     -> Ciclar modo de flujo (TICO -> EQUIDAD -> LETRERO)
 *   W     -> Salir elegantemente
 *
 * Compilar con MinGW + raylib:
 *   gcc simulacion_gui.c -o simulacion_gui.exe -I./raylib/include ^
 *       -L./raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
 */

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

/* =====================================================================
 *  CONSTANTES DE LAYOUT
 * ===================================================================== */
#define SCREEN_W     1280
#define SCREEN_H     720
#define TARGET_FPS   60

#define PANEL_TOP_H  90
#define PANEL_BOT_H  130

#define CANAL_Y      PANEL_TOP_H
#define CANAL_H      (SCREEN_H - PANEL_TOP_H - PANEL_BOT_H)  /* 500 */
#define QUEUE_W      255
#define CANAL_X      QUEUE_W
#define CANAL_W      (SCREEN_W - 2 * QUEUE_W)                /* 770 */
#define CANAL_MID_Y  (CANAL_Y + CANAL_H / 2 - 20)

#define SHIP_W       72
#define SHIP_H       34

#define MAX_QUEUE    10
#define MAX_SHIPS    60

/* =====================================================================
 *  ENUMS (equivalentes al ESP32)
 * ===================================================================== */
typedef enum { MODO_TICO = 0, MODO_EQUIDAD = 1, MODO_LETRERO = 2 } ModoFlujo;
typedef enum {
    SCH_FCFS     = 1,
    SCH_PRIORITY = 2,
    SCH_RR       = 3,
    SCH_SJF      = 4,
    SCH_STRN     = 5,
    SCH_EDF      = 6
} TipoScheduler;
typedef enum { EST_ESPERANDO = 0, EST_CRUZANDO = 1, EST_TERMINADO = 2 } EstadoBarco;

/* =====================================================================
 *  ESTRUCTURAS
 * ===================================================================== */
typedef struct {
    int          id;
    int          tipo;           /* 1=Normal, 2=Pesquero, 3=Patrulla */
    int          lado;           /* 0=izquierda, 1=derecha */
    int          tiempoCruce;    /* segundos totales */
    int          tiempoRestante; /* segundos pendientes */
    int          prioridad;      /* 1-3 */
    int          deadline;       /* para EDF */
    EstadoBarco  estado;
    float        posX, posY;     /* posicion visual actual */
    float        progreso;       /* 0..1 fraccion del cruce completado */
    float        tiempoEnSlice;  /* tiempo transcurrido en slice actual */
    int          sliceActual;    /* duracion del slice en segundos */
    bool         activo;
} Barco;

typedef struct {
    Barco *items[MAX_QUEUE];
    int    count;
} Cola;

/* =====================================================================
 *  ESTADO GLOBAL
 * ===================================================================== */
static Barco        pool[MAX_SHIPS];
static int          totalBarcos   = 0;
static int          contadorId    = 1;

static Cola         colaIzq, colaDer;
static Barco       *barcoEnCanal  = NULL;

static int          sentidoActual = 0;
static int          barcosPasados = 0;
static float        timerLetrero  = 0.0f;
static float        simTime       = 0.0f;

static ModoFlujo    modo          = MODO_TICO;
static TipoScheduler scheduler    = SCH_FCFS;
static int          quantum       = 2;
static int          paramW        = 3;
static float        tLetrero      = 5.0f;

/* Log circular */
#define LOG_LINES 9
static char         logBuf[LOG_LINES][128];
static int          logCount = 0;

/* Flag de salida */
static bool         saliendo = false;
static float        timerSalida = 0.0f;

/* =====================================================================
 *  UTILIDADES
 * ===================================================================== */
static void addLog(const char *fmt, ...) {
    if (logCount >= LOG_LINES) {
        memmove(logBuf[0], logBuf[1], (LOG_LINES - 1) * sizeof(logBuf[0]));
        logCount = LOG_LINES - 1;
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logBuf[logCount++], sizeof(logBuf[0]), fmt, ap);
    va_end(ap);
}

static Color colorTipo(int tipo) {
    if (tipo == 1) return (Color){50, 120, 230, 255};  /* azul  - Normal   */
    if (tipo == 2) return (Color){40, 170,  60, 255};  /* verde - Pesquero */
    if (tipo == 3) return (Color){210,  40,  40, 255}; /* rojo  - Patrulla */
    return GRAY;
}

static const char *strTipo(int tipo) {
    if (tipo == 1) return "Normal";
    if (tipo == 2) return "Pesquero";
    if (tipo == 3) return "Patrulla";
    return "?";
}

static const char *strScheduler(TipoScheduler s) {
    switch (s) {
        case SCH_FCFS:     return "FCFS";
        case SCH_PRIORITY: return "PRIORITY";
        case SCH_RR:       return "ROUND ROBIN";
        case SCH_SJF:      return "SJF";
        case SCH_STRN:     return "STRN";
        case SCH_EDF:      return "EDF";
    }
    return "?";
}

static const char *strModo(ModoFlujo m) {
    if (m == MODO_TICO)    return "TICO";
    if (m == MODO_EQUIDAD) return "EQUIDAD";
    return "LETRERO";
}

/* =====================================================================
 *  COLAS — sorted insert segun scheduler actual
 * ===================================================================== */
static void colaInit(Cola *c) { c->count = 0; }

static bool colaEnqueue(Cola *c, Barco *b) {
    if (c->count >= MAX_QUEUE) return false;

    int pos = c->count; /* default: al final (FCFS, RR) */

    for (int i = 0; i < c->count; i++) {
        bool antes = false;
        switch (scheduler) {
            case SCH_PRIORITY: antes = b->prioridad      > c->items[i]->prioridad;      break;
            case SCH_SJF:      antes = b->tiempoCruce    < c->items[i]->tiempoCruce;    break;
            case SCH_STRN:     antes = b->tiempoRestante < c->items[i]->tiempoRestante; break;
            case SCH_EDF:      antes = b->deadline       < c->items[i]->deadline;       break;
            default:           break; /* FCFS / RR: fifo */
        }
        if (antes) { pos = i; break; }
    }

    for (int i = c->count; i > pos; i--) c->items[i] = c->items[i - 1];
    c->items[pos] = b;
    c->count++;
    return true;
}

static Barco *colaDequeue(Cola *c) {
    if (c->count == 0) return NULL;
    Barco *b = c->items[0];
    for (int i = 0; i < c->count - 1; i++) c->items[i] = c->items[i + 1];
    c->count--;
    return b;
}

static Cola *colaActual(void) { return sentidoActual == 0 ? &colaIzq : &colaDer; }
static Cola *colaOtra(void)   { return sentidoActual == 0 ? &colaDer : &colaIzq; }

/* =====================================================================
 *  CREACION DE BARCOS
 * ===================================================================== */
static void crearBarco(int tipo, int lado) {
    if (totalBarcos >= MAX_SHIPS) { addLog("Pool lleno!"); return; }

    Barco *b = &pool[totalBarcos++];
    memset(b, 0, sizeof(Barco));

    b->id   = contadorId++;
    b->tipo = tipo;
    b->lado = lado;
    b->activo = true;
    b->estado = EST_ESPERANDO;

    switch (tipo) {
        case 1: b->tiempoCruce = 5; b->prioridad = 1; break;
        case 2: b->tiempoCruce = 4; b->prioridad = 2; break;
        case 3: b->tiempoCruce = 2; b->prioridad = 3; break;
    }
    b->tiempoRestante = b->tiempoCruce;
    b->deadline       = (int)simTime + 10 + rand() % 20;

    Cola *c = (lado == 0) ? &colaIzq : &colaDer;
    if (!colaEnqueue(c, b)) {
        addLog("Cola %s llena! Barco %d rechazado", lado == 0 ? "Izq" : "Der", b->id);
        b->activo = false;
        totalBarcos--;
        return;
    }
    addLog("Barco %d (%s, %s) -> cola", b->id, strTipo(tipo), lado == 0 ? "Izq" : "Der");
}

/* =====================================================================
 *  LOGICA DEL SCHEDULER (corre cada frame, avanza con dt)
 * ===================================================================== */
static void schedulerUpdate(float dt) {
    simTime += dt;

    /* MODO_LETRERO: conmutacion por tiempo fijo */
    if (modo == MODO_LETRERO) {
        timerLetrero += dt;
        if (timerLetrero >= tLetrero) {
            timerLetrero = 0.0f;
            sentidoActual = 1 - sentidoActual;
            barcosPasados = 0;
            addLog("[Letrero] Cambio -> %s", sentidoActual == 0 ? "Izq" : "Der");
        }
    }

    /* Actualizar barco en canal */
    if (barcoEnCanal != NULL) {
        barcoEnCanal->tiempoEnSlice += dt;

        /* Fraccion total del cruce ya realizada */
        float fracBase  = (float)(barcoEnCanal->tiempoCruce - barcoEnCanal->tiempoRestante)
                          / (float)barcoEnCanal->tiempoCruce;
        float fracSlice = (barcoEnCanal->tiempoEnSlice / (float)barcoEnCanal->sliceActual)
                          * ((float)barcoEnCanal->sliceActual / (float)barcoEnCanal->tiempoCruce);
        barcoEnCanal->progreso = fracBase + fracSlice;
        if (barcoEnCanal->progreso > 1.0f) barcoEnCanal->progreso = 1.0f;

        /* Posicion visual */
        float left  = (float)CANAL_X;
        float right = (float)(CANAL_X + CANAL_W - SHIP_W);
        if (barcoEnCanal->lado == 0)
            barcoEnCanal->posX = left  + (right - left) * barcoEnCanal->progreso;
        else
            barcoEnCanal->posX = right - (right - left) * barcoEnCanal->progreso;
        barcoEnCanal->posY = (float)CANAL_MID_Y;

        /* Slice completo? */
        if (barcoEnCanal->tiempoEnSlice >= (float)barcoEnCanal->sliceActual) {
            barcoEnCanal->tiempoRestante -= barcoEnCanal->sliceActual;

            bool preemptible = (scheduler == SCH_RR || scheduler == SCH_STRN);
            if (preemptible && barcoEnCanal->tiempoRestante > 0) {
                addLog("%s: Barco %d regresa (resta %ds)",
                    strScheduler(scheduler), barcoEnCanal->id, barcoEnCanal->tiempoRestante);
                barcoEnCanal->estado = EST_ESPERANDO;
                Cola *c = (barcoEnCanal->lado == 0) ? &colaIzq : &colaDer;
                colaEnqueue(c, barcoEnCanal);
                barcosPasados++;
            } else {
                addLog("Barco %d termino el cruce!", barcoEnCanal->id);
                barcoEnCanal->estado = EST_TERMINADO;
                barcoEnCanal->activo = false;
                barcosPasados++;
            }
            barcoEnCanal = NULL;
        }
        return; /* Canal ocupado */
    }

    /* Canal libre — decidir siguiente barco */
    bool hayIzq = colaIzq.count > 0;
    bool hayDer = colaDer.count > 0;
    if (!hayIzq && !hayDer) return;

    if (modo == MODO_TICO) {
        if (colaActual()->count == 0 && colaOtra()->count > 0) {
            sentidoActual = 1 - sentidoActual;
            addLog("[Tico] Sin barcos, cambiando a %s", sentidoActual == 0 ? "Izq" : "Der");
        }
    } else if (modo == MODO_EQUIDAD) {
        if (barcosPasados >= paramW && colaOtra()->count > 0) {
            sentidoActual = 1 - sentidoActual;
            barcosPasados = 0;
            addLog("[Equidad] W=%d alcanzado, cambiando a %s",
                paramW, sentidoActual == 0 ? "Izq" : "Der");
        } else if (colaActual()->count == 0 && colaOtra()->count > 0) {
            sentidoActual = 1 - sentidoActual;
            barcosPasados = 0;
        }
    } else { /* LETRERO */
        if (colaActual()->count == 0 && colaOtra()->count > 0) {
            sentidoActual = 1 - sentidoActual;
            barcosPasados = 0;
        }
    }

    if (colaActual()->count == 0) return;

    Barco *next = colaDequeue(colaActual());
    if (!next) return;

    next->estado        = EST_CRUZANDO;
    next->tiempoEnSlice = 0.0f;

    int slice = next->tiempoRestante;
    if (scheduler == SCH_RR && slice > quantum) slice = quantum;
    next->sliceActual = slice;

    /* Posicion visual inicial */
    float left  = (float)CANAL_X;
    float right = (float)(CANAL_X + CANAL_W - SHIP_W);
    float fracBase = (float)(next->tiempoCruce - next->tiempoRestante) / (float)next->tiempoCruce;
    if (next->lado == 0)
        next->posX = left  + (right - left) * fracBase;
    else
        next->posX = right - (right - left) * fracBase;
    next->posY = (float)CANAL_MID_Y;

    barcoEnCanal = next;
    addLog("%s -> Barco %d (%s) cruza %ds",
        strScheduler(scheduler), next->id, strTipo(next->tipo), slice);
}

/* =====================================================================
 *  DIBUJO
 * ===================================================================== */
static void drawShip(float x, float y, int tipo, int id, int lado,
                     float progreso, bool inCanal) {
    Color c    = colorTipo(tipo);
    Color dark = (Color){ c.r / 2, c.g / 2, c.b / 2, 255 };

    /* Casco */
    DrawRectangleRounded((Rectangle){ x, y, SHIP_W, SHIP_H }, 0.25f, 6, c);
    DrawRectangleRoundedLinesEx((Rectangle){ x, y, SHIP_W, SHIP_H }, 0.25f, 6, 2.0f, dark);

    /* Cabina */
    float cabX = (lado == 0) ? x + SHIP_W - 22 : x + 6;
    DrawRectangle((int)cabX, (int)(y - 10), 16, 12, dark);
    DrawRectangle((int)cabX + 2, (int)(y - 10) + 2, 12, 8, (Color){ 200, 230, 255, 200 });

    /* Chimenea con humo animado */
    float chimX = (lado == 0) ? x + SHIP_W - 14 : x + 12;
    DrawRectangle((int)chimX, (int)(y - 16), 4, 8, dark);
    if (inCanal) {
        float t = (float)GetTime() * 4.0f;
        for (int s = 0; s < 4; s++) {
            float sy  = fmodf(t * 12.0f + (float)(s * 9), 36.0f);
            float alp = 220.0f - (sy / 36.0f) * 200.0f;
            DrawCircle((int)chimX + 2, (int)(y - 16) - (int)sy,
                       4 - s / 2, (Color){ 160, 160, 160, (unsigned char)alp });
        }
    }

    /* ID */
    char lbl[8];
    snprintf(lbl, sizeof(lbl), "%d", id);
    int tw = MeasureText(lbl, 15);
    DrawText(lbl, (int)x + SHIP_W / 2 - tw / 2, (int)y + SHIP_H / 2 - 7, 15, WHITE);

    /* Barra de progreso (solo en canal) */
    if (inCanal) {
        int bx = (int)x;
        int by = (int)(y + SHIP_H + 5);
        DrawRectangle(bx, by, SHIP_W, 5, DARKGRAY);
        DrawRectangle(bx, by, (int)(SHIP_W * progreso), 5, YELLOW);
    }
}

/* Ondas animadas de agua */
static void drawWater(void) {
    float t = (float)GetTime();

    /* Fondo agua */
    DrawRectangle(CANAL_X, CANAL_Y, CANAL_W, CANAL_H, (Color){ 20, 80, 160, 255 });

    /* Ondas */
    for (int row = 0; row < 6; row++) {
        int wy = CANAL_Y + 60 + row * 70;
        for (int x = CANAL_X + 10; x < CANAL_X + CANAL_W - 20; x += 90) {
            float off = sinf(t * 1.2f + (float)x * 0.025f + (float)row * 0.8f) * 6.0f;
            DrawLineEx(
                (Vector2){ (float)x,        (float)wy + off },
                (Vector2){ (float)(x + 60), (float)wy + 3 + off },
                1.8f, (Color){ 50, 140, 220, 100 }
            );
        }
    }

    /* Bordes del canal */
    DrawRectangleLinesEx(
        (Rectangle){ CANAL_X, CANAL_Y, CANAL_W, CANAL_H }, 3, (Color){ 10, 40, 100, 255 }
    );

    /* Muelles / tierra */
    DrawRectangle(0, CANAL_Y, QUEUE_W, CANAL_H, (Color){ 120, 85, 45, 255 });
    DrawRectangle(CANAL_X + CANAL_W, CANAL_Y, QUEUE_W, CANAL_H, (Color){ 120, 85, 45, 255 });

    /* Detalles muelle: tablones */
    for (int ym = CANAL_Y + 5; ym < CANAL_Y + CANAL_H; ym += 18) {
        DrawLine(0, ym, QUEUE_W, ym, (Color){ 100, 68, 30, 80 });
        DrawLine(CANAL_X + CANAL_W, ym, SCREEN_W, ym, (Color){ 100, 68, 30, 80 });
    }

    /* Bordes muelle-agua */
    DrawRectangle(CANAL_X - 6, CANAL_Y, 6, CANAL_H, (Color){ 80, 55, 20, 255 });
    DrawRectangle(CANAL_X + CANAL_W, CANAL_Y, 6, CANAL_H, (Color){ 80, 55, 20, 255 });

    /* Flecha de direccion */
    int arrowY = CANAL_Y + 22;
    int arrowMX = CANAL_X + CANAL_W / 2;
    Color arrowC = (Color){ 255, 230, 50, 200 };
    if (sentidoActual == 0) {
        DrawText(">>>", arrowMX - 28, arrowY, 22, arrowC);
        DrawText("Izq -> Der", arrowMX - 50, arrowY + 26, 14, arrowC);
    } else {
        DrawText("<<<", arrowMX - 28, arrowY, 22, arrowC);
        DrawText("Der -> Izq", arrowMX - 50, arrowY + 26, 14, arrowC);
    }
}

static void drawQueue(Cola *cola, int lado) {
    int areaX = (lado == 0) ? 0 : CANAL_X + CANAL_W + 6;
    int areaY = CANAL_Y;

    /* Titulo */
    const char *title = (lado == 0) ? "COLA IZQ" : "COLA DER";
    DrawText(title, areaX + 8, areaY + 8, 18, WHITE);
    DrawText(TextFormat("%d barcos", cola->count), areaX + 8, areaY + 30, 13, LIGHTGRAY);

    /* Separador */
    DrawLine(areaX, areaY + 48, areaX + QUEUE_W - 6, areaY + 48, (Color){ 80, 55, 20, 200 });

    /* Barcos en cola (lista vertical) */
    for (int i = 0; i < cola->count; i++) {
        Barco *b = cola->items[i];
        int sy = areaY + 56 + i * (SHIP_H + 28);
        if (sy + SHIP_H + 28 > areaY + CANAL_H - 4) {
            DrawText(TextFormat("... +%d mas", cola->count - i), areaX + 10, sy, 13, LIGHTGRAY);
            break;
        }

        float sx;
        if (lado == 0)
            sx = (float)(areaX + QUEUE_W - SHIP_W - 14);
        else
            sx = (float)(areaX + 14);

        drawShip(sx, (float)sy, b->tipo, b->id, lado, 0.0f, false);

        /* Info extra */
        Color tc = colorTipo(b->tipo);
        DrawText(strTipo(b->tipo), (int)sx, sy - 13, 11, tc);

        char info[32];
        if (scheduler == SCH_EDF)
            snprintf(info, sizeof(info), "DL:%d", b->deadline);
        else if (scheduler == SCH_PRIORITY)
            snprintf(info, sizeof(info), "Prio:%d", b->prioridad);
        else
            snprintf(info, sizeof(info), "t:%ds", b->tiempoRestante);
        DrawText(info, (int)sx + SHIP_W + 3, sy + 8, 11, LIGHTGRAY);
    }
}

static void drawTopPanel(void) {
    DrawRectangleGradientV(0, 0, SCREEN_W, PANEL_TOP_H,
        (Color){ 18, 18, 35, 255 }, (Color){ 25, 25, 50, 255 });
    DrawLine(0, PANEL_TOP_H - 1, SCREEN_W, PANEL_TOP_H - 1, (Color){ 50, 50, 100, 255 });

    DrawText("CANAL DE BARCOS — Simulador de Scheduling", 16, 8, 22, WHITE);

    int col1 = 16, col2 = 300, col3 = 560, col4 = 820, col5 = 1050;
    int row  = 38;

    DrawText(TextFormat("Scheduler:"), col1, row, 15, GRAY);
    DrawText(strScheduler(scheduler), col1 + 80, row, 15, YELLOW);

    DrawText(TextFormat("Modo:"), col2, row, 15, GRAY);
    DrawText(strModo(modo), col2 + 50, row, 15, (Color){ 80, 220, 120, 255 });

    DrawText(TextFormat("Tiempo: %.1fs", simTime), col3, row, 15, LIGHTGRAY);

    DrawText(TextFormat("Direccion:"), col4, row, 15, GRAY);
    DrawText(sentidoActual == 0 ? "IZQ -> DER" : "DER -> IZQ",
             col4 + 82, row, 15, (Color){ 255, 200, 50, 255 });

    /* Parametro extra segun modo/scheduler */
    int px = col5;
    if (scheduler == SCH_RR)
        DrawText(TextFormat("Q=%ds", quantum), px, row, 15, ORANGE);
    if (modo == MODO_EQUIDAD)
        DrawText(TextFormat("W=%d (%d)", paramW, barcosPasados), px, row - 10, 14, ORANGE);
    if (modo == MODO_LETRERO)
        DrawText(TextFormat("Letrero: %.1f/%.0fs", timerLetrero, tLetrero), px, row, 14, ORANGE);
}

static void drawBottomPanel(void) {
    int py = CANAL_Y + CANAL_H;
    DrawRectangleGradientV(0, py, SCREEN_W, PANEL_BOT_H,
        (Color){ 12, 12, 25, 255 }, (Color){ 18, 18, 35, 255 });
    DrawLine(0, py, SCREEN_W, py, (Color){ 50, 50, 100, 255 });

    /* Controles */
    int cx = 10, cy = py + 6;
    DrawText("CONTROLES:", cx, cy, 14, GRAY);
    DrawText("[1] Normal Izq   [2] Pesquero Izq   [3] Patrulla Izq", cx, cy + 18, 13, WHITE);
    DrawText("[4] Normal Der   [5] Pesquero Der   [6] Patrulla Der", cx, cy + 34, 13, WHITE);
    DrawText("[S] Scheduler    [M] Modo    [W] Salir", cx, cy + 50, 13, WHITE);

    /* Leyenda */
    int lx = 420, ly = py + 8;
    DrawText("Leyenda:", lx, ly, 14, GRAY);
    DrawRectangle(lx,      ly + 20, 14, 14, colorTipo(1)); DrawText("Normal",   lx + 18, ly + 20, 13, WHITE);
    DrawRectangle(lx,      ly + 38, 14, 14, colorTipo(2)); DrawText("Pesquero", lx + 18, ly + 38, 13, WHITE);
    DrawRectangle(lx,      ly + 56, 14, 14, colorTipo(3)); DrawText("Patrulla", lx + 18, ly + 56, 13, WHITE);
    DrawRectangle(lx + 95, ly + 20, 14, 14, YELLOW);      DrawText("Progreso", lx + 113, ly + 20, 13, WHITE);

    /* Log */
    int logX = 620, logY = py + 4;
    DrawText("Log:", logX, logY, 13, GRAY);
    for (int i = 0; i < logCount; i++) {
        float alpha = 80.0f + 175.0f * ((float)(i + 1) / (float)logCount);
        DrawText(logBuf[i], logX, logY + 14 + i * 13, 12,
                 (Color){ 200, 200, 200, (unsigned char)alpha });
    }
}

static void drawExitOverlay(void) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 0, 0, 0, 180 });
    const char *msg = "Cerrando simulacion...";
    int tw = MeasureText(msg, 36);
    DrawText(msg, SCREEN_W / 2 - tw / 2, SCREEN_H / 2 - 18, 36, WHITE);
}

/* =====================================================================
 *  INPUT
 * ===================================================================== */
static void handleInput(void) {
    if (saliendo) return;

    if (IsKeyPressed(KEY_W)) { saliendo = true; addLog("Cerrando..."); return; }

    if (IsKeyPressed(KEY_ONE))   crearBarco(1, 0);
    if (IsKeyPressed(KEY_TWO))   crearBarco(2, 0);
    if (IsKeyPressed(KEY_THREE)) crearBarco(3, 0);
    if (IsKeyPressed(KEY_FOUR))  crearBarco(1, 1);
    if (IsKeyPressed(KEY_FIVE))  crearBarco(2, 1);
    if (IsKeyPressed(KEY_SIX))   crearBarco(3, 1);

    if (IsKeyPressed(KEY_S)) {
        scheduler = (TipoScheduler)(scheduler % 6 + 1);
        addLog("Scheduler -> %s", strScheduler(scheduler));
    }
    if (IsKeyPressed(KEY_M)) {
        modo = (ModoFlujo)((modo + 1) % 3);
        timerLetrero  = 0.0f;
        barcosPasados = 0;
        addLog("Modo -> %s", strModo(modo));
    }
}

/* =====================================================================
 *  MAIN
 * ===================================================================== */
int main(void) {
    srand((unsigned int)time(NULL));

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_W, SCREEN_H, "Canal de Barcos — Simulacion Grafica");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL); /* desactivar Escape para usar solo W */

    colaInit(&colaIzq);
    colaInit(&colaDer);

    addLog("Sistema listo. 1-3=Izq  4-6=Der  S=Sched  M=Modo  W=Salir");

    /* Demo: algunos barcos iniciales */
    crearBarco(1, 0);
    crearBarco(2, 0);
    crearBarco(3, 1);
    crearBarco(1, 1);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        handleInput();

        if (saliendo) {
            timerSalida += dt;
            if (timerSalida >= 1.5f) break;
        } else {
            schedulerUpdate(dt);
        }

        BeginDrawing();
        ClearBackground((Color){ 8, 8, 18, 255 });

        drawTopPanel();
        drawWater();
        drawQueue(&colaIzq, 0);
        drawQueue(&colaDer, 1);

        /* Barco en canal */
        if (barcoEnCanal != NULL) {
            drawShip(barcoEnCanal->posX, barcoEnCanal->posY,
                     barcoEnCanal->tipo, barcoEnCanal->id,
                     barcoEnCanal->lado, barcoEnCanal->progreso, true);
        }

        drawBottomPanel();

        if (saliendo) drawExitOverlay();

        DrawFPS(SCREEN_W - 75, 5);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
