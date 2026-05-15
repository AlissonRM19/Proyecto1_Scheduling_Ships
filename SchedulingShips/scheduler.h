#ifndef SCHEDULER_H  // Guardia de inclusión
#define SCHEDULER_H

#include "freertos/FreeRTOS.h"  // Incluir FreeRTOS
#include "freertos/semphr.h"  // Incluir semáforos
#include "barcos.h"  // Incluir barcos

#ifdef __cplusplus
extern "C" {  // Declarar como extern C si en C++
#endif

// Semáforo global para avisar que un barco terminó
extern SemaphoreHandle_t doneSemaphore;

// Inicializa el scheduler seleccionado + task principal
void iniciarScheduler();

// Operaciones de cola
bool enqueueBarco(Barco *barco);  // Encolar barco
Barco* dequeueBarco();  // Desencolar barco

void letreroTask(void *pvParameters);  // Tarea del letrero

// Task principal del scheduler
void schedulerTask(void *pvParameters);  // Tarea principal del scheduler

#ifdef __cplusplus
}
#endif

#endif // SCHEDULER_H
