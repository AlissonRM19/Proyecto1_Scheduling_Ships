#include "config.h"
#include "barcos.h"
#include "hardware.h"
#include "scheduler.h"

// ===== SETUP =====
extern "C" void actualizar_tira_led();
void setup() {
  Serial.begin(115200);
  delay(2000);

  canalMutex = xSemaphoreCreateMutex();

  if (canalMutex == NULL) {
        printf("Error: No se pudo crear el mutex del canal\n");
  }
  
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

  if (miConfig.demoMode) {
    delay(2000);
    Serial.println("Modo demo activado");
  
    crearBarco(1, 0); // Normal izquierda
    crearBarco(3, 1); // Patrulla derecha
    crearBarco(2, 0); // Pesquero izquierda
  }

  xTaskCreate(
        led_refresh_task,   // Nombre de la función
        "LED_Task",         // Nombre rastreable
        2048,               // Stack size
        NULL,               // Parámetros
        2,                  // Prioridad (menor que el scheduler)
        NULL                // Handle
    );

}



// ===== LOOP =====

void loop() {
  // Leer botones fisicos
  leerBotones();

  // Leer comandos enviados desde la GUI por serial: "ADD,tipo,lado\n"
  static char cmdBuf[32];
  static int  cmdLen = 0;
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n') {
      cmdBuf[cmdLen] = '\0';
      cmdLen = 0;
      if (cmdBuf[0]=='A' && cmdBuf[1]=='D' && cmdBuf[2]=='D' && cmdBuf[3]==',') {
        int tipo = cmdBuf[4] - '0';
        int lado = cmdBuf[6] - '0';
        if (tipo >= 1 && tipo <= 3 && (lado == 0 || lado == 1))
          crearBarco(tipo, lado);
      }
    } else if (c != '\r' && cmdLen < 30) {
      cmdBuf[cmdLen++] = c;
    }
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  delay(50);
}

void led_refresh_task(void *pvParameters) {
    while (1) {
        actualizar_tira_led(); // <--- Sin nada adentro
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
