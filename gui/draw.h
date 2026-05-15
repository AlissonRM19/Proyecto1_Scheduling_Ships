#pragma once
#include "raylib.h"
#include "types.h"

Color colorTipo(int tipo);

void drawShip(float x, float y, int tipo, int id, int lado, float progreso, bool inCanal);
void drawWater(void);
void drawQueue(Cola *cola, int lado);
void drawTopPanel(void);
void drawBottomPanel(void);
void drawExitOverlay(void);
