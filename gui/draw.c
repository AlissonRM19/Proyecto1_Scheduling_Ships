#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include "draw.h"
#include "ship_pool.h"
#include "serial.h"
#include "log.h"

Color colorTipo(int tipo) {
    if (tipo == 1) return (Color){50, 120, 230, 255};
    if (tipo == 2) return (Color){40, 170,  60, 255};
    if (tipo == 3) return (Color){210, 40,  40, 255};
    return GRAY;
}

void drawShip(float x, float y, int tipo, int id, int lado, float progreso, bool inCanal) {
    Color c = colorTipo(tipo);
    Color dark = (Color){c.r / 2, c.g / 2, c.b / 2, 255};

    DrawRectangleRounded((Rectangle){x, y, SHIP_W, SHIP_H}, 0.25f, 6, c);
    DrawRectangleRoundedLinesEx((Rectangle){x, y, SHIP_W, SHIP_H}, 0.25f, 6, 2.0f, dark);

    float cabX = (lado == 0) ? x + SHIP_W - 22 : x + 6;
    DrawRectangle((int)cabX, (int)(y - 10), 16, 12, dark);
    DrawRectangle((int)cabX + 2, (int)(y - 10) + 2, 12, 8, (Color){200, 230, 255, 200});

    float chimX = (lado == 0) ? x + SHIP_W - 14 : x + 12;
    DrawRectangle((int)chimX, (int)(y - 16), 4, 8, dark);

    if (inCanal) {
        float t = (float)GetTime() * 4.0f;
        for (int s = 0; s < 4; s++) {
            float sy  = fmodf(t * 12.0f + (float)(s * 9), 36.0f);
            float alp = 220.0f - (sy / 36.0f) * 200.0f;
            DrawCircle((int)chimX + 2, (int)(y - 16) - (int)sy, 4 - s / 2,
                       (Color){160, 160, 160, (unsigned char)alp});
        }
    }

    char lbl[8];
    snprintf(lbl, sizeof(lbl), "%d", id);
    int tw = MeasureText(lbl, 15);
    DrawText(lbl, (int)x + SHIP_W / 2 - tw / 2, (int)y + SHIP_H / 2 - 7, 15, WHITE);

    if (inCanal) {
        DrawRectangle((int)x, (int)(y + SHIP_H + 5), SHIP_W, 5, DARKGRAY);
        DrawRectangle((int)x, (int)(y + SHIP_H + 5), (int)(SHIP_W * progreso), 5, YELLOW);
    }
}

void drawWater(void) {
    float t = (float)GetTime();
    DrawRectangle(CANAL_X, CANAL_Y, CANAL_W, CANAL_H, (Color){20, 80, 160, 255});

    for (int row = 0; row < 6; row++) {
        int wy = CANAL_Y + 60 + row * 70;
        for (int x = CANAL_X + 10; x < CANAL_X + CANAL_W - 20; x += 90) {
            float off = sinf(t * 1.2f + (float)x * 0.025f + (float)row * 0.8f) * 6.0f;
            DrawLineEx((Vector2){(float)x,        (float)wy + off},
                       (Vector2){(float)(x + 60), (float)wy + 3 + off},
                       1.8f, (Color){50, 140, 220, 100});
        }
    }

    DrawRectangleLinesEx((Rectangle){CANAL_X, CANAL_Y, CANAL_W, CANAL_H}, 3, (Color){10, 40, 100, 255});
    DrawRectangle(0, CANAL_Y, QUEUE_W, CANAL_H, (Color){120, 85, 45, 255});
    DrawRectangle(CANAL_X + CANAL_W, CANAL_Y, QUEUE_W, CANAL_H, (Color){120, 85, 45, 255});

    for (int ym = CANAL_Y + 5; ym < CANAL_Y + CANAL_H; ym += 18) {
        DrawLine(0, ym, QUEUE_W, ym, (Color){100, 68, 30, 80});
        DrawLine(CANAL_X + CANAL_W, ym, SCREEN_W, ym, (Color){100, 68, 30, 80});
    }

    DrawRectangle(CANAL_X - 6, CANAL_Y, 6, CANAL_H, (Color){80, 55, 20, 255});
    DrawRectangle(CANAL_X + CANAL_W, CANAL_Y, 6, CANAL_H, (Color){80, 55, 20, 255});

    int   arrowMX = CANAL_X + CANAL_W / 2;
    Color arrowC  = (Color){255, 230, 50, 200};
    if (sentidoActual == 0) {
        DrawText(">>>",       arrowMX - 28, CANAL_Y + 22, 22, arrowC);
        DrawText("Izq -> Der",arrowMX - 50, CANAL_Y + 48, 14, arrowC);
    } else {
        DrawText("<<<",       arrowMX - 28, CANAL_Y + 22, 22, arrowC);
        DrawText("Der -> Izq",arrowMX - 50, CANAL_Y + 48, 14, arrowC);
    }
}

void drawQueue(Cola *cola, int lado) {
    int areaX = (lado == 0) ? 0 : CANAL_X + CANAL_W + 6;
    int areaY = CANAL_Y;

    DrawText((lado == 0) ? "COLA IZQ" : "COLA DER", areaX + 8, areaY + 8, 18, WHITE);
    DrawText(TextFormat("%d barcos", cola->count), areaX + 8, areaY + 30, 13, LIGHTGRAY);
    DrawLine(areaX, areaY + 48, areaX + QUEUE_W - 6, areaY + 48, (Color){80, 55, 20, 200});

    for (int i = 0; i < cola->count; i++) {
        Barco *b = cola->items[i];
        int    sy = areaY + 56 + i * (SHIP_H + 28);
        if (sy + SHIP_H + 28 > areaY + CANAL_H - 4) {
            DrawText(TextFormat("... +%d mas", cola->count - i), areaX + 10, sy, 13, LIGHTGRAY);
            break;
        }
        float sx = (lado == 0) ? (float)(areaX + QUEUE_W - SHIP_W - 14) : (float)(areaX + 14);
        drawShip(sx, (float)sy, b->tipo, b->id, lado, 0.0f, false);
        DrawText(strTipo(b->tipo), (int)sx, sy - 13, 11, colorTipo(b->tipo));

        char info[32];
        if      (scheduler == SCH_EDF)      snprintf(info, sizeof(info), "DL:%d",   b->deadline);
        else if (scheduler == SCH_PRIORITY) snprintf(info, sizeof(info), "Prio:%d", b->prioridad);
        else                                snprintf(info, sizeof(info), "t:%ds",   b->tiempoRestante);
        DrawText(info, (int)sx + SHIP_W + 3, sy + 8, 11, LIGHTGRAY);
    }
}

void drawTopPanel(void) {
    DrawRectangleGradientV(0, 0, SCREEN_W, PANEL_TOP_H,
                           (Color){18, 18, 35, 255}, (Color){25, 25, 50, 255});
    DrawLine(0, PANEL_TOP_H - 1, SCREEN_W, PANEL_TOP_H - 1, (Color){50, 50, 100, 255});
    DrawText("CANAL DE BARCOS — Scheduling Ships", 16, 8, 22, WHITE);

    Color cc = connected ? (Color){50, 220, 80, 255} : (Color){140, 140, 140, 255};
    DrawCircle(SCREEN_W - 130, 18, 7, cc);
    DrawText(connected ? "ESP32 Conectado" : "Standalone", SCREEN_W - 118, 10, 14, cc);
#ifdef __linux__
    if (connected) DrawText(serialPort, SCREEN_W - 118, 26, 11, (Color){100, 180, 100, 255});
#endif

    int row = 38;
    DrawText("Scheduler:", 16,  row, 15, GRAY);  DrawText(strScheduler(scheduler), 96,  row, 15, YELLOW);
    DrawText("Modo:",      300, row, 15, GRAY);  DrawText(strModo(modo),           350, row, 15, (Color){80, 220, 120, 255});
    DrawText(TextFormat("Tiempo:%.1fs", simTime), 560, row, 15, LIGHTGRAY);
    DrawText("Dir:",       820, row, 15, GRAY);
    DrawText(sentidoActual == 0 ? "IZQ->DER" : "DER->IZQ", 848, row, 15, (Color){255, 200, 50, 255});
    if (scheduler == SCH_RR)    DrawText(TextFormat("Q=%ds",    quantum),                   1050, row,      15, ORANGE);
    if (modo == MODO_EQUIDAD)   DrawText(TextFormat("W=%d(%d)", paramW, barcosPasados),     1050, row - 10, 14, ORANGE);
    if (modo == MODO_LETRERO)   DrawText(TextFormat("Letrero:%.1f/%.0fs", timerLetrero, tLetrero), 1050, row, 14, ORANGE);
}

void drawBottomPanel(void) {
    int py = CANAL_Y + CANAL_H;
    DrawRectangleGradientV(0, py, SCREEN_W, PANEL_BOT_H,
                           (Color){12, 12, 25, 255}, (Color){18, 18, 35, 255});
    DrawLine(0, py, SCREEN_W, py, (Color){50, 50, 100, 255});

    int cx = 10, cy = py + 6;
    DrawText("CONTROLES:",                                              cx, cy,      14, GRAY);
    DrawText("[1] Normal Izq   [2] Pesquero Izq   [3] Patrulla Izq",   cx, cy + 18, 13, WHITE);
    DrawText("[4] Normal Der   [5] Pesquero Der   [6] Patrulla Der",   cx, cy + 34, 13, WHITE);
    if (!connected) DrawText("[S] Scheduler    [M] Modo    [W] Salir", cx, cy + 50, 13, WHITE);
    else            DrawText("[W] Salir",                               cx, cy + 50, 13, WHITE);

    int lx = 420, ly = py + 8;
    DrawText("Leyenda:", lx, ly, 14, GRAY);
    DrawRectangle(lx,      ly + 20, 14, 14, colorTipo(1)); DrawText("Normal",   lx + 18,  ly + 20, 13, WHITE);
    DrawRectangle(lx,      ly + 38, 14, 14, colorTipo(2)); DrawText("Pesquero", lx + 18,  ly + 38, 13, WHITE);
    DrawRectangle(lx,      ly + 56, 14, 14, colorTipo(3)); DrawText("Patrulla", lx + 18,  ly + 56, 13, WHITE);
    DrawRectangle(lx + 95, ly + 20, 14, 14, YELLOW);       DrawText("Progreso", lx + 113, ly + 20, 13, WHITE);

    int logX = 620, logY = py + 4;
    DrawText("Log:", logX, logY, 13, GRAY);
    for (int i = 0; i < logCount; i++) {
        float alpha = 80.0f + 175.0f * ((float)(i + 1) / (float)logCount);
        DrawText(logBuf[i], logX, logY + 14 + i * 13, 12,
                 (Color){200, 200, 200, (unsigned char)alpha});
    }
}

void drawExitOverlay(void) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){0, 0, 0, 180});
    const char *msg = "Cerrando simulacion...";
    int tw = MeasureText(msg, 36);
    DrawText(msg, SCREEN_W / 2 - tw / 2, SCREEN_H / 2 - 18, 36, WHITE);
}
