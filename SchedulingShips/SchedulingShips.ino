#include <Arduino.h>

#include "config.h"
#include "barcos.h"
#include "hardware.h"
#include "scheduler.h"

// ===== SETUP =====

void setup() {
  Serial.begin(115200);
  delay(2000);

  iniciarHardware();
  iniciarScheduler();

  Serial.println();
  Serial.println("=================================");
  Serial.println("SCHEDULING SHIPS");
  Serial.println("Sistema iniciado correctamente");
  Serial.println("Presione botones para crear barcos");
  Serial.println("=================================");
  Serial.println("Scheduler actual: PRIORITY");

  if (DEMO_MODE) {
  Serial.println("Modo demo activado");

  crearBarco(1, 0); // Normal izquierda
  crearBarco(3, 1); // Patrulla derecha
  crearBarco(2, 0); // Pesquero izquierda
}

}

// ===== LOOP =====

void loop() {
  // Aquí solo se leen botones
  // y se crean barcos según el botón presionado

  leerBotones();

  // Aquí después irá:
  // ejecutarFCFS();
  // ejecutarPrioridad();
  // ejecutarRoundRobin();

  delay(100);
}
