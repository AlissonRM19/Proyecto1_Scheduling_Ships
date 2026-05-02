#ifndef SCHEDULER_H
#define SCHEDULER_H

// Sustituimos Arduino.h por las cabeceras nativas de FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "barcos.h"

#ifdef __cplusplus
extern "C" {
#endif

// Semáforo global para avisar que un barco terminó
// En C puro, SemaphoreHandle_t ya está disponible por "semphr.h"
extern SemaphoreHandle_t doneSemaphore;

// Inicializa el scheduler seleccionado + task principal
void iniciarScheduler();

// Operaciones de cola (C puro)
bool enqueueBarco(Barco *barco);
Barco* dequeueBarco();

void letreroTask(void *pvParameters);

// Task principal del scheduler
void schedulerTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // SCHEDULER_H
