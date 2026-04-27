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


//======= PINES BOTONES======
const int botonIzquierda = 4;
const int botonDerecha = 5;

// === Control IDs=====

int contadorBarcos = 1;

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
    String mensaje = "Barco " + String(barco->id) + " esperando turno...";
    Serial.println(mensaje);

    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

//========Funcion para crear barcos ===========

void crearBarco(int lado) {
  // Reservar memoria dinámica
  Barco *nuevoBarco = new Barco;

  nuevoBarco->id = contadorBarcos++;
  nuevoBarco->tipo = 1; // por ahora todos Normal
  nuevoBarco->lado = lado;
  nuevoBarco->tiempoCruce = 5;
  nuevoBarco->prioridad = 1;

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
  delay(2000);

  pinMode(botonIzquierda, INPUT_PULLUP);
  pinMode(botonDerecha, INPUT_PULLUP);

  Serial.println();
  Serial.println("=================================");
  Serial.println("SCHEDULING SHIPS");
  Serial.println("Presione botones para crear barcos");
  Serial.println("Botón GPIO 4 -> Izquierda");
  Serial.println("Botón GPIO 5 -> Derecha");
  Serial.println("=================================");
}

// ===== LOOP =====

void loop() {
  // Botón izquierda
  if (digitalRead(botonIzquierda) == LOW) {
    Serial.println("Botón izquierda presionado");
    crearBarco(0);

    delay(500); // anti rebote simple
  }

  // Botón derecha
  if (digitalRead(botonDerecha) == LOW) {
    Serial.println("Botón derecha presionado");
    crearBarco(1);

    delay(500); // anti rebote simple
  }
}
