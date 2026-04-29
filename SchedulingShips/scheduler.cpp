#include <Arduino.h>
#include "scheduler.h"
#include "barcos.h"
#include "fcfs_scheduler.h"

// ===== FCFS =====

fcfs_scheduler_t queue;
SemaphoreHandle_t queueMutex;
SemaphoreHandle_t doneSemaphore;

// ===== ENQUEUE =====

bool enqueueBarco(Barco *barco) {
  if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  bool result = fcfs_scheduler_enqueue(&queue, barco);

  xSemaphoreGive(queueMutex);
  return result;
}

// ===== DEQUEUE =====

Barco* dequeueBarco() {
  Barco *barco = NULL;

  if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
    return NULL;
  }

  if (!fcfs_scheduler_dequeue(&queue, (void**)&barco)) {
    barco = NULL;
  }

  xSemaphoreGive(queueMutex);
  return barco;
}

// ===== INIT =====

void iniciarScheduler() {
  fcfs_scheduler_init(&queue);

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

void schedulerTask(void *pvParameters) {
  while (true) {

    if (queue.count == 0) {
      vTaskDelay(200 / portTICK_PERIOD_MS);
      continue;
    }

    Barco *proximo = dequeueBarco();

    if (proximo == NULL) {
      continue;
    }

    Serial.print("FCFS -> turno para barco ");
    Serial.println(proximo->id);

    // Le da permiso al barco para cruzar
    xSemaphoreGive(proximo->sem);

    // Espera a que termine
    xSemaphoreTake(doneSemaphore, portMAX_DELAY);

    Serial.print("Barco ");
    Serial.print(proximo->id);
    Serial.println(" terminó y libera el canal");
  }
}
