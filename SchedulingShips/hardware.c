#include <stdio.h>  // Librería estándar para entrada/salida
#include "freertos/FreeRTOS.h"  // Incluir FreeRTOS
#include "freertos/task.h"  // Incluir tareas
#include "freertos/queue.h"  // Incluir colas
#include "driver/gpio.h"  // Incluir GPIO
#include "led_wrapper.h"  // Incluir wrapper de LEDs
#include "hardware.h"  // Incluir hardware
#include "barcos.h"  // Incluir barcos
#include "canal.h"  // Incluir canal
#include "config.h"  // Incluir configuración
#include "scheduler.h"  // Incluir scheduler

#include "fcfs_scheduler.h"  // Incluir FCFS
#include "priority_scheduler.h"  // Incluir Priority
#include "sjf_scheduler.h"  // Incluir SJF
#include "rr_scheduler.h"  // Incluir RR
#include "strn_scheduler.h"  // Incluir STRN
#include "edf_scheduler.h"  // Incluir EDF

extern ConfigProyecto miConfig;  // Configuración externa
extern fcfs_scheduler_t     fcfs_Izq,  fcfs_Der;  // Colas FCFS externas
extern priority_scheduler_t prio_Izq,  prio_Der;  // Colas Priority externas
extern sjf_scheduler_t      sjf_Izq,   sjf_Der;  // Colas SJF externas
extern rr_scheduler_t       rr_Izq,    rr_Der;  // Colas RR externas
extern strn_scheduler_t     strn_Izq,  strn_Der;  // Colas STRN externas
extern edf_scheduler_t      edf_Izq,   edf_Der;  // Colas EDF externas
extern Barco*               barcos_activos[MAX_BARCOS_CANAL];  // Barcos activos externos

static void configurar_boton(int pin) {  // Función para configurar botón
    gpio_reset_pin(pin);  // Resetear pin
    gpio_set_direction(pin, GPIO_MODE_INPUT);  // Establecer como entrada
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);  // Pull-up
}

// Función para iniciar hardware
void iniciarHardware() {
    printf("[Hardware] Inicializando GPIOs y LEDs...\n");  // Imprimir mensaje

    configurar_boton(BOTON_NORMAL_IZQ);  // Configurar botón normal izquierda
    configurar_boton(BOTON_PESQUERO_IZQ);  // Configurar botón pesquero izquierda
    configurar_boton(BOTON_PATRULLA_IZQ);  // Configurar botón patrulla izquierda
    configurar_boton(BOTON_NORMAL_DER);  // Configurar botón normal derecha
    configurar_boton(BOTON_PESQUERO_DER);  // Configurar botón pesquero derecha
    configurar_boton(BOTON_PATRULLA_DER);  // Configurar botón patrulla derecha

    led_init();  // Inicializar LEDs
}

// Función para mostrar tipo de barco
void mostrarTipoBarco(int tipo) {
    led_clear();  // Limpiar LEDs
    if      (tipo == 1) led_set_pixel(0, 0,   0,   255); // Normal   → Azul  // Azul para normal
    else if (tipo == 2) led_set_pixel(0, 0,   255, 0);   // Pesquero → Verde  // Verde para pesquero
    else if (tipo == 3) led_set_pixel(0, 255, 0,   0);   // Patrulla → Rojo  // Rojo para patrulla
    led_show();  // Mostrar LEDs
}

// Función helper para color por tipo
static void color_por_tipo(int tipo, int *r, int *g, int *b) {
    *r = 0; *g = 0; *b = 0;  // Inicializar colores
    if      (tipo == 1) *b = 255;   // Normal   → Azul  // Azul
    else if (tipo == 2) *g = 255;   // Pesquero → Verde  // Verde
    else if (tipo == 3) *r = 255;   // Patrulla → Rojo  // Rojo
}

// Función para actualizar tira LED
void actualizar_tira_led() {
    led_clear(); // Limpiar TODO de una vez al inicio  // Limpiar LEDs

    // Contar barcos en espera
    int esperandoIzq = 0, esperandoDer = 0;  // Contadores
    switch (SCHEDULER_ACTUAL) {
        case SCHEDULER_FCFS:     esperandoIzq = fcfs_Izq.count;  esperandoDer = fcfs_Der.count;  break;
        case SCHEDULER_PRIORITY: esperandoIzq = prio_Izq.count;  esperandoDer = prio_Der.count;  break;
        case SCHEDULER_SJF:      esperandoIzq = sjf_Izq.count;   esperandoDer = sjf_Der.count;   break;
        case SCHEDULER_RR:       esperandoIzq = rr_Izq.count;    esperandoDer = rr_Der.count;    break;
        case SCHEDULER_STRN:     esperandoIzq = strn_Izq.count;  esperandoDer = strn_Der.count;  break;
        case SCHEDULER_EDF:      esperandoIzq = edf_Izq.count;   esperandoDer = edf_Der.count;   break;
    }

    // SECCIÓN 1: Océano Izquierdo (LEDs 0-3)
    // Máximo 4 barcos visibles en la cola
    for (int i = 0; i < esperandoIzq && i < 4; i++) {
        Barco *b = NULL;  // Inicializar barco

        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS: {
                int idx = (fcfs_Izq.head + i) % FCFS_SCHEDULER_MAX_ITEMS;  // Calcular índice
                b = (Barco*)fcfs_Izq.buffer[idx];  // Obtener barco
                break;
            }
            case SCHEDULER_RR: {
                int idx = (rr_Izq.head + i) % RR_SCHEDULER_MAX_ITEMS;  // Calcular índice
                b = (Barco*)rr_Izq.buffer[idx];  // Obtener barco
                break;
            }
            case SCHEDULER_PRIORITY: b = (Barco*)prio_Izq.buffer[i];  break;  // Obtener barco
            case SCHEDULER_SJF:      b = (Barco*)sjf_Izq.buffer[i];   break;  // Obtener barco
            case SCHEDULER_STRN:     b = (Barco*)strn_Izq.buffer[i];  break;  // Obtener barco
            case SCHEDULER_EDF:      b = (Barco*)edf_Izq.buffer[i];   break;  // Obtener barco
        }

        if (b != NULL) {
            int r, g, bcolor;  // Colores
            color_por_tipo(b->tipo, &r, &g, &bcolor);  // Obtener color
            led_set_pixel(i, r, g, bcolor); // LEDs 0,1,2,3  // Establecer pixel
        }
    }

    // SECCIÓN 2: Canal (LEDs 6-10)
    // LED 5 y 11 NO se tocan — reservados para interrupciones

    // Limpiar solo los LEDs del canal
    for (int led = LED_CANAL_INICIO; led <= LED_CANAL_FIN; led++) {
        led_set_pixel(led, 0, 0, 0);  // Limpiar pixel
    }

    // Pintar barcos activos en el canal
    for (int i = 0; i < MAX_BARCOS_CANAL; i++) {
        Barco *b = barcos_activos[i];  // Obtener barco activo
        if (b == NULL) continue;  // Si nulo, continuar

        // Validar que la posición esté dentro del canal (6-10)
        // Evita pintar en LEDs reservados si hay algún valor incorrecto
        if (b->posicion < LED_CANAL_INICIO || b->posicion > LED_CANAL_FIN) continue;  // Validar posición

        int r, g, bcolor;  // Colores
        color_por_tipo(b->tipo, &r, &g, &bcolor);  // Obtener color
        led_set_pixel(b->posicion, r, g, bcolor);  // Establecer pixel
    }

    // SECCIÓN 3: Océano Derecho (LEDs 12-15)
    // LED 11 NO se toca — reservado para interrupción
    for (int i = 0; i < esperandoDer && i < 4; i++) {
        Barco *b = NULL;  // Inicializar barco

        switch (SCHEDULER_ACTUAL) {
            case SCHEDULER_FCFS: {
                int idx = (fcfs_Der.head + i) % FCFS_SCHEDULER_MAX_ITEMS;  // Calcular índice
                b = (Barco*)fcfs_Der.buffer[idx];  // Obtener barco
                break;
            }
            case SCHEDULER_RR: {
                int idx = (rr_Der.head + i) % RR_SCHEDULER_MAX_ITEMS;  // Calcular índice
                b = (Barco*)rr_Der.buffer[idx];  // Obtener barco
                break;
            }
            case SCHEDULER_PRIORITY: b = (Barco*)prio_Der.buffer[i];  break;  // Obtener barco
            case SCHEDULER_SJF:      b = (Barco*)sjf_Der.buffer[i];   break;  // Obtener barco
            case SCHEDULER_STRN:     b = (Barco*)strn_Der.buffer[i];  break;  // Obtener barco
            case SCHEDULER_EDF:      b = (Barco*)edf_Der.buffer[i];   break;  // Obtener barco
        }

        if (b != NULL) {
            int r, g, bcolor;  // Colores
            color_por_tipo(b->tipo, &r, &g, &bcolor);  // Obtener color
            led_set_pixel(LED_OCEANO_DER_INICIO + i, r, g, bcolor); // LEDs 12,13,14,15  // Establecer pixel
        }
    }

    led_show(); // Enviar todo el frame de una vez  // Mostrar LEDs
}

// Función para leer botones
void leerBotones() {
    static const struct {  // Estructura de botones
        int pin;  // Pin
        int tipo;  // Tipo
        int lado;  // Lado
        const char* nombre;  // Nombre
    } botones[] = {
        {BOTON_NORMAL_IZQ,   1, 0, "Normal Izquierda"},  // Botón normal izquierda
        {BOTON_PESQUERO_IZQ, 2, 0, "Pesquero Izquierda"},  // Botón pesquero izquierda
        {BOTON_PATRULLA_IZQ, 3, 0, "Patrulla Izquierda"},  // Botón patrulla izquierda
        {BOTON_NORMAL_DER,   1, 1, "Normal Derecha"},  // Botón normal derecha
        {BOTON_PESQUERO_DER, 2, 1, "Pesquero Derecha"},  // Botón pesquero derecha
        {BOTON_PATRULLA_DER, 3, 1, "Patrulla Derecha"}  // Botón patrulla derecha
    };

    for (int i = 0; i < 6; i++) {  // Para cada botón
        if (gpio_get_level(botones[i].pin) == 0) { // 0 = presionado (pull-up)  // Si presionado
            int r = 0, g = 0, b = 0;  // Colores
            color_por_tipo(botones[i].tipo, &r, &g, &b);  // Obtener color

            // Feedback visual en el LED del extremo correspondiente
            // Izquierda → LED 0, Derecha → LED 15
            int led_feedback = (botones[i].lado == 0)
                               ? LED_OCEANO_IZQ_INICIO
                               : LED_OCEANO_DER_FIN;  // LED de feedback
            led_set_pixel(led_feedback, r, g, b);  // Establecer pixel
            led_show();  // Mostrar

            crearBarco(botones[i].tipo, botones[i].lado);  // Crear barco

            vTaskDelay(pdMS_TO_TICKS(300)); // Debounce  // Esperar debounce

            led_clear();  // Limpiar
            led_show();  // Mostrar
        }
    }
}
