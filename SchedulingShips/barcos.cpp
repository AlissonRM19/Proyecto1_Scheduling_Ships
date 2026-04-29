#include <Arduino.h>
#include "barcos.h"

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

  while (true) {
    Serial.println("Barco "+ String(barco->id) +" Esperando turno...");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void crearBarco(int tipo, int lado) {
  Barco *nuevoBarco = new Barco;

  nuevoBarco->id = contadorBarcos++;
  nuevoBarco->tipo = tipo;
  nuevoBarco->lado = lado;
  nuevoBarco->tiempoCruce = 5;

  if (tipo == 1) nuevoBarco->prioridad = 1;
  if (tipo == 2) nuevoBarco->prioridad = 2;
  if (tipo == 3) nuevoBarco->prioridad = 3;

  xTaskCreate(
    barcoTask,
    "Barco",
    2048,
    nuevoBarco,
    nuevoBarco->prioridad,
    NULL
  );
}
