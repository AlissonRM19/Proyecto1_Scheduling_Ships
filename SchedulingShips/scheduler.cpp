#include <Arduino.h>
#include "scheduler.h"
#include "barcos.h"
#include "fcfs_scheduler.h"
#include "priority_scheduler.h"
#include "config.h"


// ===== FCFS =====

fcfs_scheduler_t fcfsQueue;
priority_scheduler_t priorityQueue;
SemaphoreHandle_t queueMutex;
SemaphoreHandle_t doneSemaphore;

// ===== ENQUEUE =====

bool enqueueBarco(Barco *barco) {
  if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  bool result = false;

  if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
    result = fcfs_scheduler_enqueue(&fcfsQueue, barco);
  }

  if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
    result = priority_scheduler_enqueue(
      &priorityQueue,
      barco,
      barco->prioridad
    );
  }

  xSemaphoreGive(queueMutex);
  return result;
}

// ===== DEQUEUE =====

Barco* dequeueBarco() {
  Barco *barco = NULL;

  if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
    return NULL;
  }

  if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
    if (!fcfs_scheduler_dequeue(&fcfsQueue, (void**)&barco)) {
      barco = NULL;
    }
  }

  if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
    if (!priority_scheduler_dequeue(&priorityQueue, (void**)&barco)) {
      barco = NULL;
    }
  }

  xSemaphoreGive(queueMutex);
  return barco;
}
// ===== INIT =====

void iniciarScheduler() {
  fcfs_scheduler_init(&fcfsQueue);
  priority_scheduler_init(&priorityQueue);

  queueMutex = xSemaphoreCreateMutex();
  doneSemaphore = xSemaphoreCreateBinary();

  xTaskCreate(
    schedulerTask,
    "Scheduler",
    4096,
    NULL,
    3,
    NULL
  );
}

// ===== TASK DEL SCHEDULER =====

// ===== TASK DEL SCHEDULER =====

void schedulerTask(void *pvParameters) {
  while (true) {

    // =========================
    // Verificar si hay barcos
    // =========================

    bool colaVacia = true;

    if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
      colaVacia = (fcfsQueue.count == 0);
    }

    if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
      colaVacia = (priorityQueue.count == 0);
    }

    if (colaVacia) {
      vTaskDelay(200 / portTICK_PERIOD_MS);
      continue;
    }

    // =========================
    // Sacar siguiente barco
    // =========================

    Barco *proximo = dequeueBarco();

    if (proximo == NULL) {
      continue;
    }

    // =========================
    // Mostrar scheduler activo
    // =========================

    if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
      Serial.print("FCFS -> turno para barco ");
      Serial.println(proximo->id);
    }

    if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
      Serial.print("PRIORITY -> turno para barco ");
      Serial.print(proximo->id);
      Serial.print(" | prioridad ");
      Serial.println(proximo->prioridad);
    }

    // =========================
    // Autorizar cruce
    // =========================

    xSemaphoreGive(proximo->sem);

    // =========================
    // Esperar que termine
    // =========================

    xSemaphoreTake(doneSemaphore, portMAX_DELAY);

    Serial.print("Barco ");
    Serial.print(proximo->id);
    Serial.println(" terminó y libera el canal");
  }
}
