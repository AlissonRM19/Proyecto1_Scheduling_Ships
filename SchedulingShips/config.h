#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

//==============================
// CONFIGURACION DE HARDWARE
//==============================

#define PIN_LED 11
#define NUM_LEDS 8

#define BOTON_NORMAL_IZQ 4
#define BOTON_PESQUERO_IZQ 5
#define BOTON_PATRULLA_IZQ 6

#define BOTON_NORMAL_DER 7
#define BOTON_PESQUERO_DER 8
#define BOTON_PATRULLA_DER 10


//==============================
// LIMITES DE LAS COLAS
//==============================
#define FCFS_SCHEDULER_MAX_ITEMS 10
#define PRIORITY_SCHEDULER_MAX_ITEMS 10
#define SJF_SCHEDULER_MAX_ITEMS 10
#define RR_SCHEDULER_MAX_ITEMS 10
#define STRN_SCHEDULER_MAX_ITEMS 10


// ============================
// ENUMS DE LÓGICA
// ============================
typedef enum { 
    MODO_EQUIDAD = 0, 
    MODO_LETRERO = 1, 
    MODO_TICO = 2 
} ModoFlujo;

typedef enum {
    SCHEDULER_FCFS = 1,
    SCHEDULER_PRIORITY = 2,
    SCHEDULER_RR = 3,
    SCHEDULER_SJF = 4,
    SCHEDULER_STRN = 5,
    SCHEDULER_EDF = 6
} TipoScheduler;

// ============================
// ESTRUCTURA GLOBAL
// ============================
typedef struct {
    ModoFlujo modoControl;      
    TipoScheduler scheduler;    
    int largoCanal;             
    int velocidadBarco;         
    int cantidadListos;         
    int parametroW;             
    int tiempoLetrero;          
    int quantumRR;              
    bool demoMode;              // Movido aquí para ser dinámico
} ConfigProyecto;

extern ConfigProyecto miConfig;

// Para mantener compatibilidad con tu código actual de ayer:
#define SCHEDULER_ACTUAL miConfig.scheduler
#define RR_QUANTUM miConfig.quantumRR 

#define DEMO_MODE miConfig.demoMode

void cargarConfiguracionInicial();

#endif
