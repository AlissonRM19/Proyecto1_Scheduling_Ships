#include "config.h"  // Incluir configuración
#include "barcos.h"  // Incluir barcos
#include "hardware.h"  // Incluir hardware
#include "scheduler.h"  // Incluir scheduler

// Función externa para actualizar tira LED
extern "C" void actualizar_tira_led();

// Función setup de Arduino
void setup() {
  Serial.begin(115200);  // Iniciar serial a 115200 baudios
  delay(2000);  // Esperar 2 segundos

  iniciarHardware();  // Iniciar hardware
  iniciarScheduler();  // Iniciar scheduler

  Serial.println();  // Imprimir línea en blanco
  Serial.println("=================================");  // Imprimir separador
  Serial.println("SCHEDULING SHIPS");  // Imprimir título
  Serial.println("Sistema iniciado correctamente");  // Imprimir mensaje
  Serial.print("Scheduler actual: ");  // Imprimir etiqueta

  switch (SCHEDULER_ACTUAL) {  // Switch según scheduler actual
    case SCHEDULER_FCFS: Serial.println("FCFS"); break;  // Caso FCFS
    case SCHEDULER_PRIORITY: Serial.println("PRIORITY"); break;  // Caso Priority
    case SCHEDULER_RR: Serial.println("RR"); break;  // Caso RR
    case SCHEDULER_SJF: Serial.println("SJF"); break;  // Caso SJF
    case SCHEDULER_STRN: Serial.println("STRN"); break;  // Caso STRN
    case SCHEDULER_EDF: Serial.println("EDF"); break;  // Caso EDF
    default: Serial.println("DESCONOCIDO"); break;  // Caso default
  }
  Serial.println("Presione botones para crear barcos");  // Imprimir instrucción
  Serial.println("=================================");  // Imprimir separador

  if (miConfig.demoMode) {  // Si modo demo activado
    delay(2000);  // Esperar 2 segundos
    Serial.println("Modo demo activado");  // Imprimir mensaje
  
    crearBarco(1, 0); // Normal izquierda  // Crear barco normal izquierda
    crearBarco(3, 1); // Patrulla derecha  // Crear barco patrulla derecha
    crearBarco(2, 0); // Pesquero izquierda  // Crear barco pesquero izquierda
  }

  xTaskCreate(  // Crear tarea para refrescar LEDs
        led_refresh_task,   // Nombre de la función
        "LED_Task",         // Nombre rastreable
        2048,               // Stack size
        NULL,               // Parámetros
        2,                  // Prioridad (menor que el scheduler)
        NULL                // Handle
    );

}

// Función loop de Arduino
void loop() {
  // Leer botones fisicos
  leerBotones();  // Leer botones físicos

  // Leer comandos enviados desde la GUI por serial: "ADD,tipo,lado\n"
  static char cmdBuf[32];  // Buffer para comando
  static int  cmdLen = 0;  // Longitud del comando
  while (Serial.available() > 0) {  // Mientras haya datos en serial
    char c = (char)Serial.read();  // Leer carácter
    if (c == '\n') {  // Si es nueva línea
      cmdBuf[cmdLen] = '\0';  // Terminar string
      cmdLen = 0;  // Resetear longitud
      if (cmdBuf[0]=='A' && cmdBuf[1]=='D' && cmdBuf[2]=='D' && cmdBuf[3]==',') {  // Si es comando ADD
        int tipo = cmdBuf[4] - '0';  // Obtener tipo
        int lado = cmdBuf[6] - '0';  // Obtener lado
        if (tipo >= 1 && tipo <= 3 && (lado == 0 || lado == 1))  // Verificar valores
          crearBarco(tipo, lado);  // Crear barco
      }
    } else if (c != '\r' && cmdLen < 30) {  // Si no es retorno y no lleno
      cmdBuf[cmdLen++] = c;  // Agregar carácter
    }
  }

  vTaskDelay(pdMS_TO_TICKS(10));  // Esperar 10 ms

  // Aquí después irá:
  // ejecutarFCFS();
  // ejecutarPrioridad();
  // ejecutarRoundRobin();

  delay(100);  // Esperar 100 ms
}

// Tarea para refrescar LEDs
void led_refresh_task(void *pvParameters) {
    while (1) {  // Bucle infinito
        actualizar_tira_led(); // <--- Sin nada adentro  // Actualizar tira LED
        vTaskDelay(pdMS_TO_TICKS(50));  // Esperar 50 ms
    }
}
