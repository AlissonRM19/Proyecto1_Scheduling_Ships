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
  Serial.print("Scheduler actual: ");

  switch (SCHEDULER_ACTUAL) {
    case SCHEDULER_FCFS: Serial.println("FCFS"); break;
    case SCHEDULER_PRIORITY: Serial.println("PRIORITY"); break;
    case SCHEDULER_RR: Serial.println("RR"); break;
    case SCHEDULER_SJF: Serial.println("SJF"); break;
    case SCHEDULER_STRN: Serial.println("STRN"); break;
    case SCHEDULER_EDF: Serial.println("EDF"); break;
    default: Serial.println("DESCONOCIDO"); break;
  }
  Serial.println("Presione botones para crear barcos");
  Serial.println("=================================");

  if (DEMO_MODE) {
    delay(2000);
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
