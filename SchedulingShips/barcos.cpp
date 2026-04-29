#include <Arduino.h>
#include "barcos.h"
#include "scheduler.h"

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

  // Espera hasta que el scheduler le dé turno
  xSemaphoreTake(barco->sem, portMAX_DELAY);

  Serial.print("Barco ");
  Serial.print(barco->id);
  Serial.println(" comienza a cruzar el canal...");

  vTaskDelay(barco->tiempoCruce * 1000 / portTICK_PERIOD_MS);

  Serial.print("Barco ");
  Serial.print(barco->id);
  Serial.println(" completó el cruce!");

  // Avisar al scheduler que terminó
  xSemaphoreGive(doneSemaphore);

  vSemaphoreDelete(barco->sem);
  delete barco;
  vTaskDelete(NULL);
}

void crearBarco(int tipo, int lado) {
  Barco *nuevoBarco = new Barco;

  nuevoBarco->id = contadorBarcos++;
  nuevoBarco->tipo = tipo;
  nuevoBarco->lado = lado;

  // NUEVO: semáforo por barco
  nuevoBarco->sem = xSemaphoreCreateBinary();

  if (nuevoBarco->sem == NULL) {
    Serial.println("Error creando semáforo");
    delete nuevoBarco;
    return;
  }

  if (tipo == 1) {
    nuevoBarco->tiempoCruce = 5;
    nuevoBarco->prioridad = 1;
  }

  if (tipo == 2) {
    nuevoBarco->tiempoCruce = 4;
    nuevoBarco->prioridad = 2;
  }

  if (tipo == 3) {
    nuevoBarco->tiempoCruce = 2;
    nuevoBarco->prioridad = 3;
  }

  // NUEVO: entra a la cola FCFS
  if (!enqueueBarco(nuevoBarco)) {
    Serial.println("Cola llena, no se puede crear barco");

    vSemaphoreDelete(nuevoBarco->sem);
    delete nuevoBarco;
    return;
  }

  Serial.print("Barco ");
  Serial.print(nuevoBarco->id);
  Serial.println(" agregado a FCFS");

  xTaskCreate(
    barcoTask,
    "Barco",
    4096,
    nuevoBarco,
    nuevoBarco->prioridad,
    NULL
  );
}
