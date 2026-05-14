#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h" // Driver nativo para el ESP32-C6
#include "led_wrapper.h"
#include "hardware.h"
#include "barcos.h"
#include "config.h"
#include "scheduler.h"


#include "fcfs_scheduler.h"
#include "priority_scheduler.h"
#include "sjf_scheduler.h"
#include "rr_scheduler.h"
#include "strn_scheduler.h"
#include "edf_scheduler.h"



extern QueueHandle_t colaIzquierda;
extern QueueHandle_t colaDerecha;
extern int sentidoActual;
extern Barco* barco_en_canal;
extern ConfigProyecto miConfig;
extern fcfs_scheduler_t fcfs_Izq, fcfs_Der;
extern priority_scheduler_t prio_Izq, prio_Der;
extern sjf_scheduler_t sjf_Izq, sjf_Der;
extern rr_scheduler_t rr_Izq, rr_Der;
extern strn_scheduler_t strn_Izq, strn_Der;
extern edf_scheduler_t edf_Izq, edf_Der;
extern Barco* barcos_activos[MAX_BARCOS_CANAL];




#define INICIO_IZQ 0
#define FIN_IZQ 3
#define BARRERAI 4
#define INICIO_CANAL 5
#define FIN_CANAL 10
#define BARRERAII 11
#define INICIO_DER 12
#define FIN_DER 15

// Función auxiliar para configurar los pines de entrada
static void configurar_boton(int pin) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}

void iniciarHardware() {
    printf("[Hardware] Inicializando GPIOs y LEDs...\n");

    // Configurar todos los botones como entrada con Pull-up
    configurar_boton(BOTON_NORMAL_IZQ);
    configurar_boton(BOTON_PESQUERO_IZQ);
    configurar_boton(BOTON_PATRULLA_IZQ);
    configurar_boton(BOTON_NORMAL_DER);
    configurar_boton(BOTON_PESQUERO_DER);
    configurar_boton(BOTON_PATRULLA_DER);

    // Inicializar el wrapper de los LEDs (definido en led_wrapper.cpp)
    led_init(); 
}

void mostrarTipoBarco(int tipo) {
    led_clear();
    // Colores: (R, G, B)
    // Usamos el LED 0 para dar feedback visual de que se presionó un botón
    if (tipo == 1) led_set_pixel(0, 0, 0, 255);      // Normal -> Azul
    else if (tipo == 2) led_set_pixel(0, 0, 255, 0); // Pesquero -> Verde
    else if (tipo == 3) led_set_pixel(0, 255, 0, 0); // Patrulla -> Rojo
    led_show();

}
// 1. Elimina el parámetro 'tipo' de la función, no lo necesita
void actualizar_tira_led() {
    led_clear(); // Solo borramos UNA VEZ al principio

    int esperandoIzq = 0;
    int esperandoDer = 0;


    // --- Determinar cuántos barcos hay ---
    if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) { esperandoIzq = fcfs_Izq.count; esperandoDer = fcfs_Der.count; }
    else if (SCHEDULER_ACTUAL == SCHEDULER_EDF) { esperandoIzq = edf_Izq.count; esperandoDer = edf_Der.count; }
    else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) { esperandoIzq = sjf_Izq.count; esperandoDer = sjf_Der.count; }
    else if (SCHEDULER_ACTUAL == SCHEDULER_RR) { esperandoIzq = rr_Izq.count; esperandoDer = rr_Der.count; }
    else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) { esperandoIzq = strn_Izq.count; esperandoDer = strn_Der.count; }
    else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) { esperandoIzq = prio_Izq.count; esperandoDer = prio_Der.count; }

    // --- SECCIÓN 1: OCÉANO IZQUIERDO (LEDs 1 al 4) ---
    for (int i = 0; i < esperandoIzq && i < 4; i++) {
        int r = 0, g = 0, b_color = 0;
        Barco* bEnEspera = NULL;
        int indiceReal = 0;
    
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
            indiceReal = (fcfs_Izq.head + i) % FCFS_SCHEDULER_MAX_ITEMS;
            bEnEspera = (Barco*)fcfs_Izq.buffer[indiceReal];
        } 
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
            indiceReal = (rr_Izq.head + i) % RR_SCHEDULER_MAX_ITEMS;
            bEnEspera = (Barco*)rr_Izq.buffer[indiceReal];
        }
        // PARA TODOS ESTOS, EL INDICE ES SIMPLEMENTE i
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF) {
            bEnEspera = (Barco*)edf_Izq.buffer[i];
        } 
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
            bEnEspera = (Barco*)sjf_Izq.buffer[i];
        } 
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
            bEnEspera = (Barco*)strn_Izq.buffer[i];
        } 
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
            bEnEspera = (Barco*)prio_Izq.buffer[i];
        }
    
        if (bEnEspera != NULL) {
            if (bEnEspera->tipo == 1) b_color = 255;      // Azul
            else if (bEnEspera->tipo == 2) g = 255; // Verde
            else if (bEnEspera->tipo == 3) r = 255; // Rojo
            
            led_set_pixel(i, r, g, b_color); 
        }
    }

    // --- SECCIÓN 2: EL CANAL (LEDs 5 al 10) ---
    // 1. Limpiamos los LEDs del canal (del 4 al 9) para que no queden rastros
    for (int i = 4; i <= 9; i++) {
        led_set_pixel(i, 0, 0, 0);
    }
    
    // 2. Pintamos TODOS los barcos que estén cruzando en este momento
    // Suponiendo que tienes acceso a los barcos que el scheduler activó:
    for (int i = 0; i < MAX_BARCOS_CANAL; i++) {
        Barco* b = barcos_activos[i];
    
        if (b != NULL) {
    
            int r = 0, g = 0, b_color = 0;
    
            if (b->tipo == 1) b_color = 255;
            else if (b->tipo == 2) g = 255;
            else if (b->tipo == 3) r = 255;
    
            if (b->posicion >= 4 && b->posicion <= 9) {
                led_set_pixel(b->posicion, r, g, b_color);
            }
        }
    }
    // --- SECCIÓN 3: OCÉANO DERECHO (LEDs 10 al 13) ---
    for (int i = 0; i < esperandoDer && i < 4; i++) {
        int r = 0, g = 0, b_color = 0;
        Barco* bEnEspera = NULL;
        int indiceReal = 0;
    
        if (SCHEDULER_ACTUAL == SCHEDULER_FCFS) {
            indiceReal = (fcfs_Der.head + i) % FCFS_SCHEDULER_MAX_ITEMS;
            bEnEspera = (Barco*)fcfs_Der.buffer[indiceReal];
        } 
        else if (SCHEDULER_ACTUAL == SCHEDULER_RR) {
            indiceReal = (rr_Der.head + i) % RR_SCHEDULER_MAX_ITEMS;
            bEnEspera = (Barco*)rr_Der.buffer[indiceReal];
        }
        else if (SCHEDULER_ACTUAL == SCHEDULER_EDF) {
            bEnEspera = (Barco*)edf_Der.buffer[i];
        } 
        else if (SCHEDULER_ACTUAL == SCHEDULER_SJF) {
            bEnEspera = (Barco*)sjf_Der.buffer[i];
        } 
        else if (SCHEDULER_ACTUAL == SCHEDULER_STRN) {
            bEnEspera = (Barco*)strn_Der.buffer[i];
        } 
        else if (SCHEDULER_ACTUAL == SCHEDULER_PRIORITY) {
            bEnEspera = (Barco*)prio_Der.buffer[i];
        }
    
        if (bEnEspera != NULL) {
    
            if (bEnEspera->tipo == 1) b_color = 255;
            else if (bEnEspera->tipo == 2) g = 255;
            else if (bEnEspera->tipo == 3) r = 255;
    
            led_set_pixel(11 + i, r, g, b_color);
        }
    }

    led_show(); // Mostramos todo el paquete de una vez
}

void leerBotones() {
    // Definimos los botones en una estructura para iterar fácilmente
    static const struct {
        int pin;
        int tipo;
        int lado;
        const char* nombre;
    } botones[] = {
        {BOTON_NORMAL_IZQ,   1, 0, "Normal Izquierda"},
        {BOTON_PESQUERO_IZQ, 2, 0, "Pesquero Izquierda"},
        {BOTON_PATRULLA_IZQ, 3, 0, "Patrulla Izquierda"},
        {BOTON_NORMAL_DER,   1, 1, "Normal Derecha"},
        {BOTON_PESQUERO_DER, 2, 1, "Pesquero Derecha"},
        {BOTON_PATRULLA_DER, 3, 1, "Patrulla Derecha"}
    };

    for (int i = 0; i < 6; i++) {
        // En el driver de ESP32, 0 significa presionado (debido al Pull-up)
        if (gpio_get_level(botones[i].pin) == 0) {
            int r=0, g=0, b=0;
            if (botones[i].tipo == 1) b=255;
            else if (botones[i].tipo == 2) g=255;
            else r=255;
        
            // Pintar el LED momentáneamente del color del barco
            led_set_pixel(botones[i].lado == 0 ? 0 : 12, r, g, b);
            led_show();
            
            crearBarco(botones[i].tipo, botones[i].lado);
            vTaskDelay(pdMS_TO_TICKS(100));
            led_clear();
            led_show();
        }
    }
}
