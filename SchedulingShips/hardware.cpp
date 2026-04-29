#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "hardware.h"
#include "barcos.h"
#include "config.h"

// ===== TIRA LED =====

Adafruit_NeoPixel tira(NUM_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);

// ===== INICIAR HARDWARE =====

void iniciarHardware() {
  // Serial NO va aquí, va en setup()

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

// ===== MOSTRAR COLOR SEGÚN TIPO =====

void mostrarTipoBarco(int tipo) {
  tira.clear();

  if (tipo == 1) {
    // Normal → Azul
    tira.setPixelColor(0, tira.Color(0, 0, 255));
  }

  if (tipo == 2) {
    // Pesquero → Verde
    tira.setPixelColor(0, tira.Color(0, 255, 0));
  }

  if (tipo == 3) {
    // Patrulla → Rojo
    tira.setPixelColor(0, tira.Color(255, 0, 0));
  }

  tira.show();
}

// ===== LEER BOTONES =====

void leerBotones() {

  // IZQUIERDA

  if (digitalRead(BOTON_NORMAL_IZQ) == LOW) {
    Serial.println("Botón Normal Izquierda");
    mostrarTipoBarco(1);
    crearBarco(1, 0);
    delay(500);
  }

  if (digitalRead(BOTON_PESQUERO_IZQ) == LOW) {
    Serial.println("Botón Pesquero Izquierda");
    mostrarTipoBarco(2);
    crearBarco(2, 0);
    delay(500);
  }

  if (digitalRead(BOTON_PATRULLA_IZQ) == LOW) {
    Serial.println("Botón Patrulla Izquierda");
    mostrarTipoBarco(3);
    crearBarco(3, 0);
    delay(500);
  }

  // DERECHA

  if (digitalRead(BOTON_NORMAL_DER) == LOW) {
    Serial.println("Botón Normal Derecha");
    mostrarTipoBarco(1);
    crearBarco(1, 1);
    delay(500);
  }

  if (digitalRead(BOTON_PESQUERO_DER) == LOW) {
    Serial.println("Botón Pesquero Derecha");
    mostrarTipoBarco(2);
    crearBarco(2, 1);
    delay(500);
  }

  if (digitalRead(BOTON_PATRULLA_DER) == LOW) {
    Serial.println("Botón Patrulla Derecha");
    mostrarTipoBarco(3);
    crearBarco(3, 1);
    delay(500);
  }
}