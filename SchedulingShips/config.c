#include "config.h"  // Incluir configuración
#include <stdbool.h>  // Incluir bool

// Inicialización de la instancia global con valores por defecto
ConfigProyecto miConfig = {
    .modoControl = MODO_TICO,  // Modo de control inicial: Tico
    .scheduler = SCHEDULER_FCFS,  // Scheduler por defecto: FCFS
    .largoCanal = 5,  // Largo del canal: 5 LEDs
    .velocidadBarco = 1000,  // Velocidad del barco: 1000 ms por LED
    .cantidadListos = 4,  // Cantidad máxima de barcos listos
    .parametroW = 2,  // Parámetro W para equidad
    .tiempoLetrero = 10,  // Tiempo del letrero: 10 segundos
    .quantumRR = 2,  // Quantum para Round Robin: 2 segundos
    .demoMode = false  // Modo demo: falso por defecto
};

void cargarConfiguracionInicial() {  // Función para cargar configuración inicial
    // Por ahora solo asegura que los valores base estén listos
    // En futuro, podría leer de memoria flash
}
