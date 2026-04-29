#ifndef CONFIG_H
#define CONFIG_H

#define PIN_LED 11
#define NUM_LEDS 8

#define BOTON_NORMAL_IZQ 4
#define BOTON_PESQUERO_IZQ 5
#define BOTON_PATRULLA_IZQ 6

#define BOTON_NORMAL_DER 7
#define BOTON_PESQUERO_DER 8
#define BOTON_PATRULLA_DER 10


// ============================
// SCHEDULERS DISPONIBLES
// ============================

#define SCHEDULER_FCFS      1
#define SCHEDULER_PRIORITY  2
#define SCHEDULER_RR        3
#define SCHEDULER_SJF       4
#define SCHEDULER_STRN      5
#define SCHEDULER_EDF       6

// ============================
// SELECCIÓN ACTUAL
// ============================

#define SCHEDULER_ACTUAL SCHEDULER_SJF

// Cambiar aquí para pruebas:
// FCFS #define SCHEDULER_ACTUAL SCHEDULER_FCFS
// PRIORITY #define SCHEDULER_ACTUAL SCHEDULER_PRIORITY
// RR #define SCHEDULER_ACTUAL SCHEDULER_RR
// SJF #define SCHEDULER_ACTUAL SCHEDULER_SJF
// STRN #define SCHEDULER_ACTUAL SCHEDULER_STRN
// EDF #define SCHEDULER_ACTUAL SCHEDULER_EDF

// ============================
// DEMO MODE
// ============================

#define DEMO_MODE true

// true  -> crea barcos automáticos
// false -> usa botones físicos


#endif
