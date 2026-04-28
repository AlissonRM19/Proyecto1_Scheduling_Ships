// Scheduling Ships — Algoritmo: Prioridad
// ESP32-C6 + Arduino IDE + FreeRTOS

#include <Arduino.h>

// ===== CONFIGURACIÓN =====

#define MAX_BARCOS_COLA 4   // máximo barcos en cada cola (según spec: max 4)

// ===== STRUCTS =====

struct Barco {
  int id;
  int tipo;             // 1=Normal, 2=Pesquero, 3=Patrulla
  int lado;             // 0=Izquierda, 1=Derecha
  int tiempoCruce;      // segundos que tarda en cruzar
  int prioridad;        // 1=baja(Normal), 2=media(Pesquero), 3=alta(Patrulla)
  SemaphoreHandle_t semCruzar;  // controlador lo da cuando es el turno del barco
  TaskHandle_t handle;
};

// ===== COLAS DE LISTOS (ordenadas por prioridad, mayor primero) =====

Barco* colaIzq[MAX_BARCOS_COLA];
int    tamColaIzq = 0;

Barco* colaDer[MAX_BARCOS_COLA];
int    tamColaDer = 0;

SemaphoreHandle_t mutexColaIzq;
SemaphoreHandle_t mutexColaDer;

// ===== ESTADO DEL CANAL =====

// Semáforo binario: empieza "dado" (canal libre).
// El controlador lo toma antes de autorizar un barco.
// El barco lo devuelve al terminar de cruzar.
SemaphoreHandle_t semCanalLibre;

// ===== CONTROL IDs =====

int contadorBarcos = 1;

// ===== PINES BOTONES =====

const int botonBNormalIzquierda    = 4;
const int botonBPesqueroIzquierda  = 5;
const int botonBPatrullaIzquierda  = 6;
const int botonBNormalDerecha      = 7;
const int botonBPesqueroDerecha    = 8;
const int botonBPatrullaDerecha    = 10;

// ===== FUNCIONES AUXILIARES =====

String nombreTipo(int tipo) {
  if (tipo == 1) return "Normal";
  if (tipo == 2) return "Pesquero";
  if (tipo == 3) return "Patrulla";
  return "Desconocido";
}

String nombreLado(int lado) {
  return lado == 0 ? "Izquierda" : "Derecha";
}

// ===== COLA DE PRIORIDAD: INSERTAR =====
// Mantiene la cola ordenada de mayor a menor prioridad (insertion sort).
// Cuando llega un Patrulla (prioridad 3) siempre queda de primero.

void insertarEnCola(Barco* barco) {
  Barco**           cola;
  int*              tam;
  SemaphoreHandle_t mutex;

  if (barco->lado == 0) {
    cola  = colaIzq;
    tam   = &tamColaIzq;
    mutex = mutexColaIzq;
  } else {
    cola  = colaDer;
    tam   = &tamColaDer;
    mutex = mutexColaDer;
  }

  xSemaphoreTake(mutex, portMAX_DELAY);

  if (*tam >= MAX_BARCOS_COLA) {
    Serial.println("[COLA LLENA] No se puede agregar barco");
    xSemaphoreGive(mutex);
    return;
  }

  // Insertion sort descendente por prioridad
  int i = *tam;
  while (i > 0 && cola[i - 1]->prioridad < barco->prioridad) {
    cola[i] = cola[i - 1];
    i--;
  }
  cola[i] = barco;
  (*tam)++;

  xSemaphoreGive(mutex);
}

// ===== COLA DE PRIORIDAD: SACAR EL PRIMERO =====
// Extrae y retorna el barco de mayor prioridad (índice 0).

Barco* sacarPrimero(Barco** cola, int* tam, SemaphoreHandle_t mutex) {
  xSemaphoreTake(mutex, portMAX_DELAY);

  if (*tam == 0) {
    xSemaphoreGive(mutex);
    return NULL;
  }

  Barco* barco = cola[0];
  for (int i = 0; i < *tam - 1; i++) {
    cola[i] = cola[i + 1];
  }
  (*tam)--;

  xSemaphoreGive(mutex);
  return barco;
}

// ===== IMPRIMIR COLA =====

void imprimirCola(const char* nombre, Barco** cola, int tam) {
  Serial.print("[COLA ");
  Serial.print(nombre);
  Serial.print("] ");
  if (tam == 0) {
    Serial.println("(vacia)");
    return;
  }
  for (int i = 0; i < tam; i++) {
    Serial.print("B");
    Serial.print(cola[i]->id);
    Serial.print("(P");
    Serial.print(cola[i]->prioridad);
    Serial.print("-");
    Serial.print(nombreTipo(cola[i]->tipo));
    Serial.print(")");
    if (i < tam - 1) Serial.print(" > ");
  }
  Serial.println();
}

// ===== TASK DE CADA BARCO =====

void barcoTask(void* pvParameters) {
  Barco* barco = (Barco*) pvParameters;

  Serial.println("=================================");
  Serial.print("Barco creado  ID: ");     Serial.println(barco->id);
  Serial.print("Tipo: ");                 Serial.println(nombreTipo(barco->tipo));
  Serial.print("Lado: ");                 Serial.println(nombreLado(barco->lado));
  Serial.print("Tiempo de cruce: ");      Serial.print(barco->tiempoCruce); Serial.println("s");
  Serial.print("Prioridad: ");            Serial.println(barco->prioridad);
  Serial.println("Estado: En cola de listos");

  // Insertarse en la cola de prioridad
  insertarEnCola(barco);

  // Mostrar estado de ambas colas tras la inserción
  imprimirCola("IZQ", colaIzq, tamColaIzq);
  imprimirCola("DER", colaDer, tamColaDer);

  // Bloquear hasta que el controlador del canal autorice cruzar
  xSemaphoreTake(barco->semCruzar, portMAX_DELAY);

  // === CRUZAR EL CANAL ===
  Serial.print("[CRUZANDO] Barco ");
  Serial.print(barco->id);
  Serial.print(" (");
  Serial.print(nombreTipo(barco->tipo));
  Serial.print(") cruza de ");
  Serial.print(nombreLado(barco->lado));
  Serial.println(" -> otro lado");

  vTaskDelay((barco->tiempoCruce * 1000) / portTICK_PERIOD_MS);

  Serial.print("[LLEGO] Barco ");
  Serial.print(barco->id);
  Serial.println(" llego al otro oceano");

  // Liberar el canal para el siguiente barco
  xSemaphoreGive(semCanalLibre);

  // Limpiar y salir
  vSemaphoreDelete(barco->semCruzar);
  delete barco;
  vTaskDelete(NULL);
}

// ===== TASK CONTROLADOR DEL CANAL — Algoritmo Prioridad =====
// Compara el frente de ambas colas y autoriza el barco de mayor prioridad global.
// Si hay empate de prioridad, prefiere el lado que tenga barcos (o izquierda).

void canalControladorTask(void* pvParameters) {
  while (true) {

    // Esperar a que haya al menos un barco en alguna cola
    if (tamColaIzq == 0 && tamColaDer == 0) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    // Comparar prioridades de los frentes de ambas colas
    int priorIzq = (tamColaIzq > 0) ? colaIzq[0]->prioridad : -1;
    int priorDer = (tamColaDer > 0) ? colaDer[0]->prioridad : -1;

    Barco* siguiente = NULL;

    if (priorIzq >= priorDer && priorIzq != -1) {
      siguiente = sacarPrimero(colaIzq, &tamColaIzq, mutexColaIzq);
    } else if (priorDer != -1) {
      siguiente = sacarPrimero(colaDer, &tamColaDer, mutexColaDer);
    }

    if (siguiente == NULL) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    // Tomar el canal (bloquea hasta que el barco anterior termine de cruzar)
    xSemaphoreTake(semCanalLibre, portMAX_DELAY);

    Serial.println("---------------------------------");
    Serial.print("[CANAL] Autorizando -> Barco ");
    Serial.print(siguiente->id);
    Serial.print(" | Tipo: ");
    Serial.print(nombreTipo(siguiente->tipo));
    Serial.print(" | Prioridad: ");
    Serial.println(siguiente->prioridad);

    // Mostrar colas restantes
    imprimirCola("IZQ", colaIzq, tamColaIzq);
    imprimirCola("DER", colaDer, tamColaDer);
    Serial.println("---------------------------------");

    // Señalar al barco que puede cruzar
    xSemaphoreGive(siguiente->semCruzar);
    // semCanalLibre será devuelto por el barco al terminar de cruzar
  }
}

// ===== CREAR BARCO =====

void crearBarco(int tipo, int lado) {
  Barco* nuevoBarco = new Barco;
  nuevoBarco->id        = contadorBarcos++;
  nuevoBarco->tipo      = tipo;
  nuevoBarco->lado      = lado;
  nuevoBarco->semCruzar = xSemaphoreCreateBinary();
  nuevoBarco->handle    = NULL;

  // Tiempo de cruce y prioridad según tipo
  if (tipo == 1) { nuevoBarco->tiempoCruce = 5; nuevoBarco->prioridad = 1; }
  if (tipo == 2) { nuevoBarco->tiempoCruce = 4; nuevoBarco->prioridad = 2; }
  if (tipo == 3) { nuevoBarco->tiempoCruce = 2; nuevoBarco->prioridad = 3; }

  // Todos los barcos con misma prioridad FreeRTOS: el orden lo maneja la cola
  xTaskCreate(barcoTask, "Barco", 2048, nuevoBarco, 1, &nuevoBarco->handle);
}

// ===== SETUP =====

void setup() {
  Serial.begin(115200);
  delay(1000);  // reducido para simulacion en Wokwi

  // Semáforos de las colas
  mutexColaIzq = xSemaphoreCreateMutex();
  mutexColaDer = xSemaphoreCreateMutex();

  // Canal empieza libre
  semCanalLibre = xSemaphoreCreateBinary();
  xSemaphoreGive(semCanalLibre);

  // Pines botones
  pinMode(botonBNormalIzquierda,   INPUT_PULLUP);
  pinMode(botonBPesqueroIzquierda, INPUT_PULLUP);
  pinMode(botonBPatrullaIzquierda, INPUT_PULLUP);
  pinMode(botonBNormalDerecha,     INPUT_PULLUP);
  pinMode(botonBPesqueroDerecha,   INPUT_PULLUP);
  pinMode(botonBPatrullaDerecha,   INPUT_PULLUP);

  // Task controlador del canal (prioridad 2 > barcos en prioridad 1)
  xTaskCreate(canalControladorTask, "CanalCtrl", 4096, NULL, 2, NULL);

  Serial.println("=================================");
  Serial.println("SCHEDULING SHIPS — Algoritmo: Prioridad");
  Serial.println("Prioridad: Patrulla(3) > Pesquero(2) > Normal(1)");
  Serial.println("Btn1=NormalIzq  Btn2=PesqueroIzq  Btn3=PatrullaIzq");
  Serial.println("Btn4=NormalDer  Btn5=PesqueroDer  Btn6=PatrullaDer");
  Serial.println("=================================");
}

// ===== LOOP =====

void loop() {
  if (digitalRead(botonBNormalIzquierda)   == LOW) { crearBarco(1, 0); delay(300); }
  if (digitalRead(botonBPesqueroIzquierda) == LOW) { crearBarco(2, 0); delay(300); }
  if (digitalRead(botonBPatrullaIzquierda) == LOW) { crearBarco(3, 0); delay(300); }
  if (digitalRead(botonBNormalDerecha)     == LOW) { crearBarco(1, 1); delay(300); }
  if (digitalRead(botonBPesqueroDerecha)   == LOW) { crearBarco(2, 1); delay(300); }
  if (digitalRead(botonBPatrullaDerecha)   == LOW) { crearBarco(3, 1); delay(300); }
}
