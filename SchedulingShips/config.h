#ifndef CONFIG_H  // Guardia de inclusión
#define CONFIG_H

#include <stdint.h>  // Incluir tipos estándar
#include <stdbool.h>  // Incluir bool

// Configuración de hardware
#define PIN_LED 11  // Pin para LEDs
#define NUM_LEDS 8  // Número de LEDs

#define BOTON_NORMAL_IZQ 4  // Botón normal izquierda
#define BOTON_PESQUERO_IZQ 5  // Botón pesquero izquierda
#define BOTON_PATRULLA_IZQ 6  // Botón patrulla izquierda

#define BOTON_NORMAL_DER 7  // Botón normal derecha
#define BOTON_PESQUERO_DER 8  // Botón pesquero derecha
#define BOTON_PATRULLA_DER 10  // Botón patrulla derecha

// Límites de las colas
#define FCFS_SCHEDULER_MAX_ITEMS 10  // Máximo items para FCFS
#define PRIORITY_SCHEDULER_MAX_ITEMS 10  // Máximo items para Priority
#define SJF_SCHEDULER_MAX_ITEMS 10  // Máximo items para SJF
#define RR_SCHEDULER_MAX_ITEMS 10  // Máximo items para RR
#define STRN_SCHEDULER_MAX_ITEMS 10  // Máximo items para STRN

// Enums de lógica
typedef enum { 
    MODO_EQUIDAD = 0,  // Modo equidad
    MODO_LETRERO = 1,  // Modo letrero
    MODO_TICO = 2  // Modo tico
} ModoFlujo;

typedef enum {
    SCHEDULER_FCFS = 1,  // Scheduler FCFS
    SCHEDULER_PRIORITY = 2,  // Scheduler Priority
    SCHEDULER_RR = 3,  // Scheduler Round Robin
    SCHEDULER_SJF = 4,  // Scheduler SJF
    SCHEDULER_STRN = 5,  // Scheduler STRN
    SCHEDULER_EDF = 6  // Scheduler EDF
} TipoScheduler;

// Estructura global
typedef struct {
    ModoFlujo modoControl;  // Modo de control del flujo
    TipoScheduler scheduler;  // Tipo de scheduler
    int largoCanal;  // Largo del canal
    int velocidadBarco;  // Velocidad del barco
    int cantidadListos;  // Cantidad de barcos listos
    int parametroW;  // Parámetro W
    int tiempoLetrero;  // Tiempo del letrero
    int quantumRR;  // Quantum para Round Robin
    bool demoMode;  // Modo demo
} ConfigProyecto;

extern ConfigProyecto miConfig;  // Configuración global

#define SCHEDULER_ACTUAL miConfig.scheduler  // Macro para scheduler actual
#define RR_QUANTUM miConfig.quantumRR  // Macro para quantum RR
#define DEMO_MODE miConfig.demoMode  // Macro para demo mode

void cargarConfiguracionInicial();  // Función para cargar configuración inicial

#define MAX_BARCOS_CANAL 3  // Máximo barcos en canal

#endif
