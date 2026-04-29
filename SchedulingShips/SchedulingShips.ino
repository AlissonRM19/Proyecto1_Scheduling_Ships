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
const int botonBNormalIzquierda = 4;
const int botonBPesqueroIzquierda = 5;
const int botonBPatrullaIzquierda = 6;


const int botonBNormalDerecha = 7;
const int botonBPesqueroDerecha = 8;
const int botonBPatrullaDerecha = 10;



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

void crearBarco(int tipo, int lado) {
  // Reservar memoria dinámica
  Barco *nuevoBarco = new Barco;

  nuevoBarco->id = contadorBarcos++;
  nuevoBarco->tipo = tipo;
  nuevoBarco->lado = lado;

  //tiempos segun el tipo de barco
  if (tipo == 1){
    nuevoBarco->tiempoCruce = 5;
    nuevoBarco->prioridad = 1;
  }
  if (tipo == 2){
    nuevoBarco->tiempoCruce = 4;
    nuevoBarco->prioridad = 2;
  }
  if (tipo == 3){
    nuevoBarco->tiempoCruce = 2;
    nuevoBarco->prioridad = 3;
  }
  

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

  pinMode(botonBNormalIzquierda, INPUT_PULLUP);
  pinMode(botonBPesqueroIzquierda, INPUT_PULLUP);
  pinMode(botonBPatrullaIzquierda, INPUT_PULLUP);

  pinMode(botonBNormalDerecha, INPUT_PULLUP);
  pinMode(botonBPesqueroDerecha, INPUT_PULLUP);
  pinMode(botonBPatrullaDerecha, INPUT_PULLUP);

  Serial.println();
  Serial.println("=================================");
  Serial.println("SCHEDULING SHIPS");
  Serial.println("Presione los botones para crear barcos");
  Serial.println("Botón 1 -> Barco Normal en la Izquierda");
  Serial.println("Botón 2 -> Barco Pesquero en la Izquierda");
  Serial.println("Botón 3 -> Barco Patrulla en la Izquierda");
  Serial.println("Botón 4 -> Barco Normal en la Derecha");
  Serial.println("Botón 5 -> Barco Pesquero en la Derecha");
  Serial.println("Botón 6 -> Barco Patrulla en la Derecha");
  Serial.println("=================================");
}

// ===== LOOP =====
void loop() {

  if (digitalRead(botonBNormalIzquierda) == LOW) {
    crearBarco(1, 0);
    delay(300);
  }

  if (digitalRead(botonBPesqueroIzquierda) == LOW) {
    crearBarco(2, 0);
    delay(300);
  }

  if (digitalRead(botonBPatrullaIzquierda) == LOW) {
    crearBarco(3, 0);
    delay(300);
  }

  if (digitalRead(botonBNormalDerecha) == LOW) {
    crearBarco(1, 1);
    delay(300);
  }

  if (digitalRead(botonBPesqueroDerecha) == LOW) {
    crearBarco(2, 1);
    delay(300);
  }

  if (digitalRead(botonBPatrullaDerecha) == LOW) {
    crearBarco(3, 1);
    delay(300);
  }
}
