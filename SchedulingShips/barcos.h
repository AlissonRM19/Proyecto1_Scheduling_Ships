#ifndef BARCOS_H
#define BARCOS_H

#include <Arduino.h>

struct Barco {
  int id;
  int tipo;          // 1 = Normal, 2 = Pesquero, 3 = Patrulla
  int lado;          // 0 = Izquierda, 1 = Derecha
  int tiempoCruce;
  int prioridad;

  SemaphoreHandle_t sem;
};

String nombreTipo(int tipo);
String nombreLado(int lado);

void barcoTask(void *pvParameters);
void crearBarco(int tipo, int lado);

#endif
