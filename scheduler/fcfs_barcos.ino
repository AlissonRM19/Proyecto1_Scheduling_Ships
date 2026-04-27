// Scheduling Ships
// ESP32-C6 + Arduino IDE + FreeRTOS
// Calendarizador: FCFS (First Come, First Served)
// El barco que llega primero al canal es el primero en cruzar.
// No hay desalojo: una vez que un barco empieza a cruzar, lo termina.

#include <Arduino.h>

// ===== CONSTANTES =====
#define MAX_COLA     8       // Máximo de barcos en cada cola de listos
#define LADO_IZQ     0
#define LADO_DER     1

// ===== ESTRUCTURA BARCO =====
struct Barco {
  int id;
  int tipo;          // 1 = Normal, 2 = Pesquero, 3 = Patrulla
  int lado;          // 0 = Oceano izquierdo, 1 = Oceano derecho
  int tiempoCruce;   // Segundos que tarda en cruzar el canal
  int prioridad;     // Prioridad FreeRTOS de su task
  int ordenLlegada;  // Número de orden de llegada (clave del FCFS)
};

// ===== PINES BOTONES =====
const int botonBNormalIzquierda   = 4;
const int botonBPesqueroIzquierda = 5;
const int botonBPatrullaIzquierda = 6;

const int botonBNormalDerecha     = 7;
const int botonBPesqueroDerecha   = 8;
const int botonBPatrullaDerecha   = 10;

// ===== CONTROL GLOBAL =====
int contadorBarcos    = 1;   // ID único de cada barco
int contadorLlegada   = 1;   // Orden de llegada global (FCFS key)

// ===== COLA FCFS (arreglo circular simple) =====
// Una cola por lado: izquierda [0] y derecha [1]
Barco* colaFCFS[2][MAX_COLA];
int    frente[2]    = {0, 0};
int    fin[2]       = {0, 0};
int    tamCola[2]   = {0, 0};

// ===== SEMÁFOROS Y MUTEX =====
SemaphoreHandle_t mutexCola;      // Protege acceso a las colas
SemaphoreHandle_t mutexCanal;     // Solo un barco cruza a la vez
SemaphoreHandle_t semFCFS[2];     // Señal al scheduler por lado

// ===== FUNCIONES AUXILIARES =====

String nombreTipo(int tipo) {
  if (tipo == 1) return "Normal";
  if (tipo == 2) return "Pesquero";
  if (tipo == 3) return "Patrulla";
  return "Desconocido";
}

String nombreLado(int lado) {
  if (lado == LADO_IZQ) return "Izquierda";
  return "Derecha";
}

// Encola un barco en la cola FCFS del lado correspondiente
// Retorna true si tuvo éxito, false si la cola está llena
bool encolarBarco(Barco* barco) {
  int lado = barco->lado;

  if (xSemaphoreTake(mutexCola, portMAX_DELAY) == pdTRUE) {
    if (tamCola[lado] >= MAX_COLA) {
      Serial.println("[FCFS] Cola llena, barco descartado.");
      xSemaphoreGive(mutexCola);
      return false;
    }
    colaFCFS[lado][fin[lado]] = barco;
    fin[lado]   = (fin[lado] + 1) % MAX_COLA;
    tamCola[lado]++;

    Serial.print("[FCFS] Barco ");
    Serial.print(barco->id);
    Serial.print(" encolado en lado ");
    Serial.print(nombreLado(lado));
    Serial.print(" | Orden de llegada: ");
    Serial.println(barco->ordenLlegada);

    xSemaphoreGive(mutexCola);

    // Avisar al scheduler de este lado que hay un barco nuevo
    xSemaphoreGive(semFCFS[lado]);
    return true;
  }
  return false;
}

// Saca el primer barco de la cola (FCFS = el de menor ordenLlegada = el primero encolado)
Barco* desencolarBarco(int lado) {
  Barco* barco = nullptr;
  if (xSemaphoreTake(mutexCola, portMAX_DELAY) == pdTRUE) {
    if (tamCola[lado] > 0) {
      barco = colaFCFS[lado][frente[lado]];
      frente[lado]  = (frente[lado] + 1) % MAX_COLA;
      tamCola[lado]--;
    }
    xSemaphoreGive(mutexCola);
  }
  return barco;
}

// Imprime el estado actual de las colas (diagnóstico)
void imprimirColas() {
  if (xSemaphoreTake(mutexCola, portMAX_DELAY) == pdTRUE) {
    Serial.println("----- Estado de colas FCFS -----");
    for (int lado = 0; lado < 2; lado++) {
      Serial.print("  Cola ");
      Serial.print(nombreLado(lado));
      Serial.print(": [");
      int idx = frente[lado];
      for (int i = 0; i < tamCola[lado]; i++) {
        Barco* b = colaFCFS[lado][(idx + i) % MAX_COLA];
        Serial.print("ID");
        Serial.print(b->id);
        Serial.print("(llegada:");
        Serial.print(b->ordenLlegada);
        Serial.print(")");
        if (i < tamCola[lado] - 1) Serial.print(" -> ");
      }
      Serial.println("]");
    }
    Serial.println("--------------------------------");
    xSemaphoreGive(mutexCola);
  }
}

// ===== TASK DE CADA BARCO =====
// El barco solo se registra y se encola; el canal lo ejecuta el scheduler
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
  Serial.print(barco->tiempoCruce);
  Serial.println(" s");
  Serial.print("Orden de llegada (FCFS): ");
  Serial.println(barco->ordenLlegada);
  Serial.println("Estado: Encolado - esperando turno FCFS");
  Serial.println("=================================");

  // Encolar en la cola FCFS del lado correspondiente
  encolarBarco(barco);

  // El task del barco termina aquí; el scheduler FCFS lo ejecutará
  vTaskDelete(NULL);
}

// ===== TASK SCHEDULER FCFS (uno por lado) =====
// Espera barcos en su cola y los manda a cruzar en orden de llegada
void schedulerFCFS(void *pvParameters) {
  int lado = (int)(intptr_t) pvParameters;

  Serial.print("[Scheduler FCFS] Iniciado para lado ");
  Serial.println(nombreLado(lado));

  while (true) {
    // Esperar señal de que hay al menos un barco en la cola
    xSemaphoreTake(semFCFS[lado], portMAX_DELAY);

    Barco* barco = desencolarBarco(lado);
    if (barco == nullptr) continue;

    // Tomar control exclusivo del canal (mutex)
    Serial.print("[FCFS] Barco ");
    Serial.print(barco->id);
    Serial.print(" (");
    Serial.print(nombreTipo(barco->tipo));
    Serial.print(") espera acceso al canal desde ");
    Serial.println(nombreLado(lado));

    xSemaphoreTake(mutexCanal, portMAX_DELAY);

    // ---- CRUZANDO EL CANAL ----
    Serial.println("=================================");
    Serial.print("[CANAL] Barco ");
    Serial.print(barco->id);
    Serial.print(" CRUZANDO | Tipo: ");
    Serial.print(nombreTipo(barco->tipo));
    Serial.print(" | De: ");
    Serial.print(nombreLado(lado));
    Serial.print(" -> ");
    Serial.println(nombreLado(1 - lado));
    Serial.print("[CANAL] Tiempo estimado: ");
    Serial.print(barco->tiempoCruce);
    Serial.println(" s");
    Serial.println("=================================");

    imprimirColas();

    // Simular el tiempo de cruce
    vTaskDelay((barco->tiempoCruce * 1000) / portTICK_PERIOD_MS);

    // ---- LLEGÓ AL OTRO LADO ----
    Serial.print("[CANAL] Barco ");
    Serial.print(barco->id);
    Serial.println(" CRUZÓ exitosamente.");

    // Liberar el canal y la memoria del barco
    xSemaphoreGive(mutexCanal);
    delete barco;
  }
}

// ===== FUNCIÓN PARA CREAR BARCOS =====
void crearBarco(int tipo, int lado) {
  Barco *nuevoBarco = new Barco;

  nuevoBarco->id           = contadorBarcos++;
  nuevoBarco->tipo         = tipo;
  nuevoBarco->lado         = lado;
  nuevoBarco->ordenLlegada = contadorLlegada++;   // Clave FCFS

  // Tiempos y prioridades según tipo
  if (tipo == 1) {                  // Normal (más lento)
    nuevoBarco->tiempoCruce = 5;
    nuevoBarco->prioridad   = 1;
  } else if (tipo == 2) {           // Pesquero
    nuevoBarco->tiempoCruce = 4;
    nuevoBarco->prioridad   = 2;
  } else {                          // Patrulla (más rápido)
    nuevoBarco->tiempoCruce = 2;
    nuevoBarco->prioridad   = 3;
  }

  // Crear task del barco (se encola y se autoeliminan)
  xTaskCreate(
    barcoTask,
    "Barco",
    2048,
    nuevoBarco,
    nuevoBarco->prioridad,
    NULL
  );
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(4000);

  // Inicializar colas
  for (int i = 0; i < 2; i++) {
    frente[i]  = 0;
    fin[i]     = 0;
    tamCola[i] = 0;
  }

  // Crear semáforos y mutex
  mutexCola  = xSemaphoreCreateMutex();
  mutexCanal = xSemaphoreCreateMutex();
  semFCFS[LADO_IZQ] = xSemaphoreCreateCounting(MAX_COLA, 0);
  semFCFS[LADO_DER] = xSemaphoreCreateCounting(MAX_COLA, 0);

  // Pines botones
  pinMode(botonBNormalIzquierda,   INPUT_PULLUP);
  pinMode(botonBPesqueroIzquierda, INPUT_PULLUP);
  pinMode(botonBPatrullaIzquierda, INPUT_PULLUP);
  pinMode(botonBNormalDerecha,     INPUT_PULLUP);
  pinMode(botonBPesqueroDerecha,   INPUT_PULLUP);
  pinMode(botonBPatrullaDerecha,   INPUT_PULLUP);

  // Lanzar schedulers FCFS (uno por lado)
  xTaskCreate(schedulerFCFS, "SchedulerFCFS_IZQ", 4096, (void*)LADO_IZQ, 4, NULL);
  xTaskCreate(schedulerFCFS, "SchedulerFCFS_DER", 4096, (void*)LADO_DER, 4, NULL);

  Serial.println();
  Serial.println("=================================");
  Serial.println("SCHEDULING SHIPS — FCFS");
  Serial.println("Algoritmo: First Come, First Served");
  Serial.println("El primer barco en llegar es el primero en cruzar.");
  Serial.println("---------------------------------");
  Serial.println("Botón 1 -> Barco Normal    | Izquierda");
  Serial.println("Botón 2 -> Barco Pesquero  | Izquierda");
  Serial.println("Botón 3 -> Barco Patrulla  | Izquierda");
  Serial.println("Botón 4 -> Barco Normal    | Derecha");
  Serial.println("Botón 5 -> Barco Pesquero  | Derecha");
  Serial.println("Botón 6 -> Barco Patrulla  | Derecha");
  Serial.println("=================================");
}

// ===== LOOP =====
void loop() {
  if (digitalRead(botonBNormalIzquierda) == LOW) {
    crearBarco(1, LADO_IZQ);
    delay(300);
  }
  if (digitalRead(botonBPesqueroIzquierda) == LOW) {
    crearBarco(2, LADO_IZQ);
    delay(300);
  }
  if (digitalRead(botonBPatrullaIzquierda) == LOW) {
    crearBarco(3, LADO_IZQ);
    delay(300);
  }
  if (digitalRead(botonBNormalDerecha) == LOW) {
    crearBarco(1, LADO_DER);
    delay(300);
  }
  if (digitalRead(botonBPesqueroDerecha) == LOW) {
    crearBarco(2, LADO_DER);
    delay(300);
  }
  if (digitalRead(botonBPatrullaDerecha) == LOW) {
    crearBarco(3, LADO_DER);
    delay(300);
  }
}