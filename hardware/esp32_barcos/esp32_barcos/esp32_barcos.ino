// Scheduling Ships
// ESP32-C6 + Arduino IDE + FreeRTOS
// FCFS scheduler para barcos en cola de listos

#include <Arduino.h>
#include "fcfs_scheduler.h"

struct Barco {
  int id;
  int tipo;          // 1 = Normal, 2 = Pesquero, 3 = Patrulla
  int lado;          // 0 = Oceano de la izquierda, 1 = Oceano de la derecha
  int tiempoCruce;
  int prioridad;
  SemaphoreHandle_t sem;
};

// ===== PINES BOTONES =====
const int botonBNormalIzquierda = 4;
const int botonBPesqueroIzquierda = 5;
const int botonBPatrullaIzquierda = 6;

const int botonBNormalDerecha = 7;
const int botonBPesqueroDerecha = 8;
const int botonBPatrullaDerecha = 10;

// ===== FCFS Queue =====
fcfs_scheduler_t queue;
SemaphoreHandle_t queueMutex;
SemaphoreHandle_t doneSemaphore;
int contadorBarcos = 1;

String nombreTipo(int tipo) {
  if (tipo == 1) return "Normal";
  if (tipo == 2) return "Pesquero";
  if (tipo == 3) return "Patrulla";
  return "Desconocido";
}

String nombreLado(int lado) {
  if (lado == 0) return "Izquierda";
  return "Derecha";
}

bool enqueueBarco(Barco *barco) {
  if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  bool result = fcfs_scheduler_enqueue(&queue, barco);
  xSemaphoreGive(queueMutex);
  return result;
}

Barco *dequeueBarco() {
  Barco *barco = NULL;
  if (xSemaphoreTake(queueMutex, portMAX_DELAY) != pdTRUE) {
    return NULL;
  }

  if (!fcfs_scheduler_dequeue(&queue, (void **)&barco)) {
    barco = NULL;
  }

  xSemaphoreGive(queueMutex);
  return barco;
}

void imprimirEstadoCola() {
  if (queue.count == 0) {
    Serial.println("Cola FCFS vacía");
    return;
  }

  Serial.print("Cola FCFS (orden de llegada): ");
  int index = queue.head;
  for (int i = 0; i < queue.count; i++) {
    Barco *b = (Barco *)queue.buffer[index];
    Serial.print(b->id);
    Serial.print((i < queue.count - 1) ? " -> " : "\n");
    index = (index + 1) % FCFS_SCHEDULER_MAX_ITEMS;
  }
}

void barcoTask(void *pvParameters) {
  Barco *barco = (Barco *) pvParameters;

  Serial.println("=================================");
  Serial.print("Barco creado -> ID: ");
  Serial.println(barco->id);
  Serial.print("Tipo: ");
  Serial.println(nombreTipo(barco->tipo));
  Serial.print("Lado de origen: ");
  Serial.println(nombreLado(barco->lado));
  Serial.print("Tiempo de cruce: ");
  Serial.println(barco->tiempoCruce);
  Serial.print("Prioridad: ");
  Serial.println(barco->prioridad);
  Serial.println("Estado: En cola de listos");
  Serial.println("=================================");

  // Espera hasta que el scheduler FCFS le de el turno.
  xSemaphoreTake(barco->sem, portMAX_DELAY);

  Serial.print("Barco ");
  Serial.print(barco->id);
  Serial.println(" comienza a cruzar el canal...");

  vTaskDelay(barco->tiempoCruce * 1000 / portTICK_PERIOD_MS);

  Serial.print("Barco ");
  Serial.print(barco->id);
  Serial.println(" completó el cruce!");

  xSemaphoreGive(doneSemaphore);
  vSemaphoreDelete(barco->sem);
  delete barco;
  vTaskDelete(NULL);
}

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

    Serial.print("Scheduler FCFS: asignando turno al barco ");
    Serial.println(proximo->id);
    imprimirEstadoCola();
    xSemaphoreGive(proximo->sem);

    xSemaphoreTake(doneSemaphore, portMAX_DELAY);
    Serial.print("Scheduler FCFS: el barco ");
    Serial.print(proximo->id);
    Serial.println(" terminó y libera el canal.");
  }
}

void crearBarco(int tipo, int lado) {
  Barco *nuevoBarco = new Barco;
  nuevoBarco->id = contadorBarcos++;
  nuevoBarco->tipo = tipo;
  nuevoBarco->lado = lado;
  nuevoBarco->sem = xSemaphoreCreateBinary();

  if (nuevoBarco->sem == NULL) {
    Serial.println("Error al crear semáforo de barco");
    delete nuevoBarco;
    return;
  }

  if (tipo == 1) {
    nuevoBarco->tiempoCruce = 5;
    nuevoBarco->prioridad = 1;
  } else if (tipo == 2) {
    nuevoBarco->tiempoCruce = 4;
    nuevoBarco->prioridad = 2;
  } else if (tipo == 3) {
    nuevoBarco->tiempoCruce = 2;
    nuevoBarco->prioridad = 3;
  }

  if (!enqueueBarco(nuevoBarco)) {
    Serial.println("Cola FCFS llena: no se puede crear más barcos");
    vSemaphoreDelete(nuevoBarco->sem);
    delete nuevoBarco;
    return;
  }

  Serial.print("Barco ");
  Serial.print(nuevoBarco->id);
  Serial.println(" encolado en FCFS");

  xTaskCreate(
    barcoTask,
    "Barco",
    4096,
    nuevoBarco,
    nuevoBarco->prioridad,
    NULL
  );
}

void setup() {
  Serial.begin(115200);
  delay(4000);

  pinMode(botonBNormalIzquierda, INPUT_PULLUP);
  pinMode(botonBPesqueroIzquierda, INPUT_PULLUP);
  pinMode(botonBPatrullaIzquierda, INPUT_PULLUP);
  pinMode(botonBNormalDerecha, INPUT_PULLUP);
  pinMode(botonBPesqueroDerecha, INPUT_PULLUP);
  pinMode(botonBPatrullaDerecha, INPUT_PULLUP);

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

  Serial.println();
  Serial.println("=================================");
  Serial.println("SCHEDULING SHIPS - FCFS");
  Serial.println("Presione los botones para crear barcos");
  Serial.println("Botón 1 -> Barco Normal en la Izquierda");
  Serial.println("Botón 2 -> Barco Pesquero en la Izquierda");
  Serial.println("Botón 3 -> Barco Patrulla en la Izquierda");
  Serial.println("Botón 4 -> Barco Normal en la Derecha");
  Serial.println("Botón 5 -> Barco Pesquero en la Derecha");
  Serial.println("Botón 6 -> Barco Patrulla en la Derecha");
  Serial.println("=================================");
}

void loop() {
  if (digitalRead(botonBNormalIzquierda) == LOW) {
    crearBarco(1, 0);
    delay(300);
  }
  if (digitalRead(botonBPesqueroIzquierda) == LOW) {
    crearBarco(2, 0);
    delay(300);
  }
  if (digitalRead(botonBPatrullaIzquierda) == LOW) {
    crearBarco(3, 0);
    delay(300);
  }
  if (digitalRead(botonBNormalDerecha) == LOW) {
    crearBarco(1, 1);
    delay(300);
  }
  if (digitalRead(botonBPesqueroDerecha) == LOW) {
    crearBarco(2, 1);
    delay(300);
  }
  if (digitalRead(botonBPatrullaDerecha) == LOW) {
    crearBarco(3, 1);
    delay(300);
  }

  vTaskDelay(100 / portTICK_PERIOD_MS);
}
