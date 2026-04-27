// Scheduling Ships
// ESP32-C6 + Arduino IDE + FreeRTOS
// Parte 1: Crear barcos como tasks

struct Barco {
  int id;
  int tipo;          // 1 = Normal, 2 = Pesquero, 3 = Patrulla
  int lado;          // 0 = Oceano de la izquierda, 1 = Oceano de la derecha
  int tiempoCruce;
  int prioridad;
};

// ===== FUNCIONES AUXILIARES =====

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

// ===== TASK DE CADA BARCO =====

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

  // Simulación continua
  while (true) {
    Serial.print("Barco ");
    Serial.print(barco->id);
    Serial.println(" esperando turno...");

    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

// ===== SETUP =====

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("=================================");
  Serial.println("INICIANDO SCHEDULING SHIPS");
  Serial.println("ESP32-C6 + FreeRTOS");
  Serial.println("=================================");

  // ===== CREACIÓN DE BARCOS =====

  // static evita problemas de memoria con FreeRTOS

  static Barco barco1 = {
    1,  // ID
    1,  // Normal
    0,  // Izquierda
    5,  // Tiempo cruce
    1   // Prioridad
  };

  static Barco barco2 = {
    2,  // ID
    3,  // Patrulla
    1,  // Derecha
    2,  // Tiempo cruce
    3   // Prioridad
  };

  static Barco barco3 = {
    3,  // ID
    2,  // Pesquero
    0,  // Izquierda
    3,  // Tiempo cruce
    2   // Prioridad
  };

  // ===== CREAR TASKS =====

  xTaskCreate(
    barcoTask,
    "Barco1",
    2048,
    &barco1,
    barco1.prioridad,
    NULL
  );

  xTaskCreate(
    barcoTask,
    "Barco2",
    2048,
    &barco2,
    barco2.prioridad,
    NULL
  );

  xTaskCreate(
    barcoTask,
    "Barco3",
    2048,
    &barco3,
    barco3.prioridad,
    NULL
  );

  Serial.println("Todos los barcos fueron creados.");
}

// ===== LOOP =====

void loop() {
  // No usamos loop()
}
