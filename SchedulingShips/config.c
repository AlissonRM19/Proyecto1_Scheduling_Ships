#include "config.h"
#include <stdbool.h>

// Inicialización de la instancia global con valores por defecto
// Estos son los valores que el sistema usará al arrancar
ConfigProyecto miConfig = {
    .modoControl = MODO_TICO,   // Empieza con modo tiempo
    .scheduler = SCHEDULER_FCFS,         // Algoritmo por defecto: First-Come, First-Served
    .largoCanal = 5,               // 5 LEDs de largo para el canal
    .velocidadBarco = 2000,        // 2 segundos para moverse entre LEDs
    .cantidadListos = 4,           // Máximo 4 barcos por lado
    .parametroW = 2,               // Para modo Equidad (pasan 2 y 2)
    .tiempoLetrero = 10,           // 10 segundos por cada sentido
    .quantumRR = 2,                // 2 segundos de ráfaga para RR
    .demoMode = false              // Empezar con botones físicos por defecto
};

void cargarConfiguracionInicial() {
    // Por ahora solo se asegura que los valores base estén listos.
    // En un futuro, aquí se podria leer de la memoria flash del ESP32
    // si se quisiera que el sistema recordara la configuración al apagarse.
}
