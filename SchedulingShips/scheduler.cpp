#include <Arduino.h>
#include "scheduler.h"
#include "barcos.h"
#include "fcfs_scheduler.h"
#include "priority_scheduler.h"
#include "sjf_scheduler.h"
#include "rr_scheduler.h"
#include "strn_scheduler.h"
#include "config.h"


// ===== COLAS =====

fcfs_scheduler_t fcfsQueue;
priority_scheduler_t priorityQueue;
sjf_scheduler_t sjfQueue;
rr_scheduler_t rrQueue;
strn_scheduler_t strnQueue;
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

  if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
    result = sjf_scheduler_enqueue(
      &sjfQueue,
      barco,
      barco->tiempoCruce
    );
  }
  
  if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
    result = rr_scheduler_enqueue(&rrQueue, barco);
  }

  if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
    result = strn_scheduler_enqueue(
      &strnQueue,
      barco,
      barco->tiempoRestante
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

  if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
    if (!sjf_scheduler_dequeue(&sjfQueue, (void**)&barco)) {
      barco = NULL;
    }
  }
  
  if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
    if (!rr_scheduler_dequeue(&rrQueue, (void**)&barco)) {
      barco = NULL;
    }
  }
  
  if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
    if (!strn_scheduler_dequeue(&strnQueue, (void**)&barco)) {
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
  sjf_scheduler_init(&sjfQueue);
  rr_scheduler_init(&rrQueue);
  strn_scheduler_init(&strnQueue);

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

    if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
      colaVacia = (sjfQueue.count == 0);
    }
    
    if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
      colaVacia = (rrQueue.count == 0);
    }
    
    if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
      colaVacia = (strnQueue.count == 0);
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

    if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
      Serial.print("SJF -> turno para barco ");
      Serial.print(proximo->id);
      Serial.print(" | tiempo de cruce ");
      Serial.print(proximo->tiempoCruce);
    }
    
    if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
      Serial.print("RR -> turno para barco ");
      Serial.print(proximo->id);
      Serial.print(" | tiempo restante ");
      Serial.print(proximo->tiempoRestante);
      Serial.print("s | quantum ");
      Serial.print(RR_QUANTUM);
      Serial.println("s");
    }

    if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
      Serial.print("STRN -> turno para barco ");
      Serial.print(proximo->id);
      Serial.print(" | tiempo restante ");
      Serial.print(proximo->tiempoRestante);
      Serial.println("s");
    }
    // =========================
    // Autorizar cruce
    // =========================

    xSemaphoreGive(proximo->sem);

    // =========================
    // Esperar que termine
    // =========================

    xSemaphoreTake(doneSemaphore, portMAX_DELAY);

    // RR y STRN pueden re-encolar
    if (
      (SCHEDULER_ACTUAL == SCHEDULER_RR ||
       SCHEDULER_ACTUAL == SCHEDULER_STRN)
      &&
      proximo->tiempoRestante > 0
    ) {

      if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
        Serial.print("RR -> barco ");
      } else {
        Serial.print("STRN -> barco ");
      }

      Serial.print(proximo->id);
      Serial.println(" regresa a la cola con tiempo restante");

      enqueueBarco(proximo);
    }
    else {
      Serial.print("Barco ");
      Serial.print(proximo->id);
      Serial.println(" terminó y libera el canal");
    }

  } 
}
