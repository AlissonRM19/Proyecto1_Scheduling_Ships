#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include "ship_pool.h"
#include "serial.h"
#include "scheduler.h"
#include "draw.h"
#include "log.h"

static bool  saliendo    = false;
static float timerSalida = 0.0f;

static void handleInput(void) {
    if (saliendo) return;
    if (IsKeyPressed(KEY_W)) { saliendo = true; addLog("Cerrando..."); return; }

    if (IsKeyPressed(KEY_ONE))   { if (connected) serialSend("ADD,1,0\n"); else crearBarco(1, 0); }
    if (IsKeyPressed(KEY_TWO))   { if (connected) serialSend("ADD,2,0\n"); else crearBarco(2, 0); }
    if (IsKeyPressed(KEY_THREE)) { if (connected) serialSend("ADD,3,0\n"); else crearBarco(3, 0); }
    if (IsKeyPressed(KEY_FOUR))  { if (connected) serialSend("ADD,1,1\n"); else crearBarco(1, 1); }
    if (IsKeyPressed(KEY_FIVE))  { if (connected) serialSend("ADD,2,1\n"); else crearBarco(2, 1); }
    if (IsKeyPressed(KEY_SIX))   { if (connected) serialSend("ADD,3,1\n"); else crearBarco(3, 1); }

    if (!connected) {
        if (IsKeyPressed(KEY_S)) {
            scheduler = (TipoScheduler)(scheduler % 6 + 1);
            addLog("Scheduler -> %s", strScheduler(scheduler));
        }
        if (IsKeyPressed(KEY_M)) {
            modo = (ModoFlujo)((modo + 1) % 3);
            timerLetrero = 0.0f;
            barcosPasados = 0;
            addLog("Modo -> %s", strModo(modo));
        }
    }
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));

    connected = serialInit(argc, argv);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_W, SCREEN_H, "Canal de Barcos — Simulacion Grafica");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);

    colaInit(&colaIzq);
    colaInit(&colaDer);

    if (connected) {
        addLog("ESP32 conectado en %s", serialPort);
    } else {
        addLog("Standalone. 1-3=Izq 4-6=Der S=Sched M=Modo W=Salir");
        crearBarco(1, 0); crearBarco(2, 0); crearBarco(3, 1); crearBarco(1, 1);
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        serialPollRead();
        handleInput();
        if (saliendo) {
            timerSalida += dt;
            if (timerSalida >= 1.5f) break;
        } else {
            schedulerUpdate(dt);
        }

        BeginDrawing();
        ClearBackground((Color){8, 8, 18, 255});
        drawTopPanel();
        drawWater();
        drawQueue(&colaIzq, 0);
        drawQueue(&colaDer,  1);
        if (barcoEnCanal)
            drawShip(barcoEnCanal->posX, barcoEnCanal->posY,
                     barcoEnCanal->tipo, barcoEnCanal->id,
                     barcoEnCanal->lado, barcoEnCanal->progreso, true);
        drawBottomPanel();
        if (saliendo) drawExitOverlay();
        DrawFPS(SCREEN_W - 75, 5);
        EndDrawing();
    }

    serialClose();
    CloseWindow();
    return 0;
}
