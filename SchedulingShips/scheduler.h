#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include "barcos.h"

// Semáforo global para avisar que un barco terminó
extern SemaphoreHandle_t doneSemaphore;

// Inicializa FCFS + task del scheduler
void iniciarScheduler();

// Operaciones de cola
bool enqueueBarco(Barco *barco);
Barco* dequeueBarco();

// Task principal del scheduler
void schedulerTask(void *pvParameters);

#endif
