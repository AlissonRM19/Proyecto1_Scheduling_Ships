#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "hardware.h"
#include "barcos.h"
#include "config.h"

// ===== TIRA LED =====

Adafruit_NeoPixel tira(NUM_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);

// ===== INICIAR HARDWARE =====

void iniciarHardware() {
  pinMode(BOTON_NORMAL_IZQ, INPUT_PULLUP);
  pinMode(BOTON_PESQUERO_IZQ, INPUT_PULLUP);
  pinMode(BOTON_PATRULLA_IZQ, INPUT_PULLUP);

  pinMode(BOTON_NORMAL_DER, INPUT_PULLUP);
  pinMode(BOTON_PESQUERO_DER, INPUT_PULLUP);
  pinMode(BOTON_PATRULLA_DER, INPUT_PULLUP);

  tira.begin();
  tira.clear();
  tira.show();
}



//LEDS
void apagarCanal() {
  tira.clear();
  tira.show();
}

// ===== MOSTRAR ZONA Y COLOR SEGÚN TIPO =====
void mostrarZonaBarco(int tipo, int zona) {
  tira.clear();

  uint32_t color = 0;

  // Colores por tipo
  if (tipo == 1) {
    color = tira.Color(0, 0, 255); // Normal → azul
  }

  if (tipo == 2) {
    color = tira.Color(0, 255, 0); // Pesquero → verde
  }

  if (tipo == 3) {
    color = tira.Color(255, 0, 0); // Patrulla → rojo
  }

  int inicio = 0;
  int fin = 0;

  // Zonas
  // zona 0 = izquierda
  // zona 1 = canal
  // zona 2 = derecha

  if (zona == 0) {
    inicio = 0;
    fin = 3;
  }

  if (zona == 1) {
    inicio = 4;
    fin = 8;
  }

  if (zona == 2) {
    inicio = 9;
    fin = 12;
  }

  for (int i = inicio; i <= fin; i++) {
    tira.setPixelColor(i, color);
  }

  tira.show();
}

void mostrarBarcoEnCanal(int tipo, int lado) {
  tira.clear();

  uint32_t color = 0;

  // Color según tipo
  if (tipo == 1) {
    color = tira.Color(0, 0, 255); // Normal = azul
  }

  if (tipo == 2) {
    color = tira.Color(0, 255, 0); // Pesquero = verde
  }

  if (tipo == 3) {
    color = tira.Color(255, 0, 0); // Patrulla = rojo
  }

  // =========================
  // 1. Encender lado de origen
  // =========================

  if (lado == 0) {
    for (int i = 0; i <= 3; i++) {
      tira.setPixelColor(i, color);
    }
  }

  if (lado == 1) {
    for (int i = 9; i <= 12; i++) {
      tira.setPixelColor(i, color);
    }
  }

  tira.show();
  delay(300);

  // =========================
  // 2. Recorrer canal LED por LED
  // =========================

  for (int i = 4; i <= 8; i++) {
    tira.setPixelColor(i, color);
    tira.show();
    delay(200);
  }

  // =========================
  // 3. Encender lado destino
  // =========================

  if (lado == 0) {
    for (int i = 9; i <= 12; i++) {
      tira.setPixelColor(i, color);
    }
  }

  if (lado == 1) {
    for (int i = 0; i <= 3; i++) {
      tira.setPixelColor(i, color);
    }
  }

  tira.show();
  delay(500);

  // =========================
  // 4. Apagar todo
  // =========================

  tira.clear();
  tira.show();
}

// ===== LEER BOTONES =====

void leerBotones() {

  // IZQUIERDA

  if (digitalRead(BOTON_NORMAL_IZQ) == LOW) {
    Serial.println("Botón Normal Izquierda");
    mostrarBarcoEnCanal(1, 0);
    crearBarco(1, 0);
    delay(500);
  }

  if (digitalRead(BOTON_PESQUERO_IZQ) == LOW) {
    Serial.println("Botón Pesquero Izquierda");
    mostrarBarcoEnCanal(2, 0);
    crearBarco(2, 0);
    delay(500);
  }

  if (digitalRead(BOTON_PATRULLA_IZQ) == LOW) {
    Serial.println("Botón Patrulla Izquierda");
    mostrarBarcoEnCanal(3, 0);
    crearBarco(3, 0);
    delay(500);
  }

  // DERECHA

  if (digitalRead(BOTON_NORMAL_DER) == LOW) {
    Serial.println("Botón Normal Derecha");
    mostrarBarcoEnCanal(1, 1);
    crearBarco(1, 1);
    delay(500);
  }

  if (digitalRead(BOTON_PESQUERO_DER) == LOW) {
    Serial.println("Botón Pesquero Derecha");
    mostrarBarcoEnCanal(2, 1);
    crearBarco(2, 1);
    delay(500);
  }

  if (digitalRead(BOTON_PATRULLA_DER) == LOW) {
    Serial.println("Botón Patrulla Derecha");
    mostrarBarcoEnCanal(3, 1);
    crearBarco(3, 1);
    delay(500);
  }
}
