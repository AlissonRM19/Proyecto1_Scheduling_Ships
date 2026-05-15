#include <stdio.h>  // Librería estándar para funciones de entrada y salida
#include <stdlib.h>  // Librería estándar para funciones de memoria y conversión
#include "freertos/FreeRTOS.h"  // Incluir FreeRTOS para tareas y semáforos
#include "freertos/task.h"  // Incluir tareas de FreeRTOS
#include "freertos/semphr.h"  // Incluir semáforos de FreeRTOS
#include "barcos.h"  // Incluir cabecera de barcos
#include "canal.h"  // Incluir cabecera del canal
#include "config.h"  // Incluir configuración
#include "scheduler.h"  // Incluir planificador

int contadorBarcos = 1;  // Contador global para asignar IDs únicos a los barcos

// Variables externas declaradas en otros módulos
extern int                barcos_en_canal_count;  // Contador de barcos en el canal
extern int                sentido_actual;  // Sentido actual del canal
extern Barco*             barcos_activos[MAX_BARCOS_CANAL];  // Array de barcos activos
extern SemaphoreHandle_t  sem_espacio_canal;  // Semáforo para espacio en el canal
extern SemaphoreHandle_t  doneSemaphore;  // Semáforo para indicar finalización

// Función para obtener el nombre del tipo de barco basado en un entero
const char* nombreTipo(int tipo) {
    switch (tipo) {
        case 1:  return "Normal";  // Tipo normal
        case 2:  return "Pesquero";  // Tipo pesquero
        case 3:  return "Patrulla";  // Tipo patrulla
        default: return "Desconocido";  // Tipo desconocido
    }
}

// Función para obtener el nombre del lado basado en un entero
const char* nombreLado(int lado) {
    return (lado == 0) ? "Izquierda" : "Derecha";  // 0 para izquierda, 1 para derecha
}

// Tarea principal del barco
void barcoTask(void *pvParameters) {
    Barco *barco      = (Barco *) pvParameters;  // Obtener el puntero al barco desde parámetros
    int slot_asignado = -1;  // Inicializar slot asignado a -1

    xSemaphoreTake(barco->sem, portMAX_DELAY);  // Tomar el semáforo del barco para esperar turno

    // Asignar slot en barcos_activos
    for (int i = 0; i < MAX_BARCOS_CANAL; i++) {
        if (barcos_activos[i] == NULL) {
            barcos_activos[i] = barco;
            slot_asignado     = i;
            break;
        }
    }

    // Imprimir mensaje de entrada al canal
    printf("[Barco %d] %s desde %s entra al canal\n",
           barco->id, nombreTipo(barco->tipo), nombreLado(barco->lado));

    // Bucle mientras el barco tenga tiempo restante
    while (barco->tiempoRestante > 0) {

        // Para RR/STRN: esperar re-despacho después de cada slice
        if (barco->tiempoRestante < barco->tiempoCruce) {
            xSemaphoreTake(barco->sem, portMAX_DELAY);
        }

        int slice = barco->tiempoRestante;  // Inicializar slice con tiempo restante
        if (SCHEDULER_ACTUAL == SCHEDULER_RR && slice > RR_QUANTUM) {
            slice = RR_QUANTUM;  // Limitar slice para Round Robin
        }

        // Determinar dirección de LEDs
        int led_inicio, led_fin, paso;
        if (barco->lado == 0) {        // Izquierda → Derecha
            led_inicio = LED_CANAL_INICIO;  // LED inicial
            led_fin    = LED_CANAL_FIN;     // LED final
            paso       = 1;  // Paso positivo
        } else {                        // Derecha → Izquierda
            led_inicio = LED_CANAL_FIN;     // LED inicial
            led_fin    = LED_CANAL_INICIO;  // LED final
            paso       = -1;  // Paso negativo
        }

        // Calcular delay por LED basado en velocidad del barco
        int delay_por_led = (barco->tiempoCruce * 1000) / LED_CANAL_TOTAL;

        // Calcular LEDs a recorrer en este slice
        int leds_en_slice = (LED_CANAL_TOTAL * slice) / barco->tiempoCruce;
        if (leds_en_slice < 1) leds_en_slice = 1;
        if (leds_en_slice > LED_CANAL_TOTAL) leds_en_slice = LED_CANAL_TOTAL;

        // Determinar LED actual de inicio del slice
        int led_actual = (barco->tiempoRestante == barco->tiempoCruce)
                         ? led_inicio
                         : barco->posicion;

        // Ocupar primera posición antes de entrar al canal
        canal_ocupar_posicion(canal_pos_a_index(led_actual));
        barco->posicion = led_actual;
        printf("GUI:POS,%d,%d\n", barco->id, barco->posicion);

        // Bucle para recorrer LEDs en el slice
        for (int s = 0; s < leds_en_slice; s++) {

            vTaskDelay(pdMS_TO_TICKS(delay_por_led));  // Esperar delay por LED

            int led_siguiente = led_actual + paso;  // Calcular siguiente LED
            bool hay_siguiente = (paso == 1)
                                  ? (led_siguiente <= led_fin)
                                  : (led_siguiente >= led_fin);  // Verificar si hay siguiente

            if (hay_siguiente) {
                // Ocupar siguiente antes de liberar actual
                canal_ocupar_posicion(canal_pos_a_index(led_siguiente));
                canal_liberar_posicion(canal_pos_a_index(led_actual));

                led_actual      = led_siguiente;
                barco->posicion = led_actual;  // Actualizar posición
                printf("GUI:POS,%d,%d\n", barco->id, barco->posicion);
            } else {
                // Último LED: liberar y salir
                canal_liberar_posicion(canal_pos_a_index(led_actual));
            }
        }

        barco->tiempoRestante -= slice;  // Reducir tiempo restante
        xSemaphoreGive(doneSemaphore);  // Dar semáforo de done
    }

    printf("GUI:DONE,%d\n", barco->id);
    printf("[Barco %d] Cruce completado\n", barco->id);

    // Liberar slot
    if (slot_asignado != -1) {
        barcos_activos[slot_asignado] = NULL;
    }

    barcos_en_canal_count--;  // Decrementar contador de barcos en canal
    xSemaphoreGive(sem_espacio_canal);  // Liberar semáforo de espacio

    vSemaphoreDelete(barco->sem);  // Eliminar semáforo del barco
    free(barco);  // Liberar memoria del barco
    vTaskDelete(NULL);  // Eliminar tarea
}

// Función para crear un nuevo barco
void crearBarco(int tipo, int lado) {
    Barco *nuevoBarco = (Barco*) malloc(sizeof(Barco));  // Asignar memoria para nuevo barco
    if (nuevoBarco == NULL) {
        printf("[ERROR] Sin memoria para barco\n");
        return;
    }

    // Establecer posición inicial
    nuevoBarco->posicion = (lado == 0) ? 4 : 12;
    nuevoBarco->id       = contadorBarcos++;  // Asignar ID único
    nuevoBarco->tipo     = tipo;
    nuevoBarco->lado     = lado;

    nuevoBarco->sem = xSemaphoreCreateBinary();  // Crear semáforo binario
    if (nuevoBarco->sem == NULL) {
        printf("[ERROR] Sin semáforo para barco\n");
        free(nuevoBarco);
        return;
    }

    // Establecer tiempo de cruce y prioridad según tipo
    switch (tipo) {
        case 1: nuevoBarco->tiempoCruce = 5; nuevoBarco->prioridad = 1; break;
        case 2: nuevoBarco->tiempoCruce = 4; nuevoBarco->prioridad = 2; break;
        case 3: nuevoBarco->tiempoCruce = 2; nuevoBarco->prioridad = 3; break;
        default: nuevoBarco->tiempoCruce = 5; nuevoBarco->prioridad = 1; break;
    }

    nuevoBarco->tiempoRestante = nuevoBarco->tiempoCruce;  // Inicializar tiempo restante
    nuevoBarco->deadline       = (int)(pdTICKS_TO_MS(xTaskGetTickCount()) / 1000)
                                 + (rand() % 20 + 10);  // Calcular deadline aleatorio

    if (!enqueueBarco(nuevoBarco)) {  // Intentar encolar el barco
        printf("[WARN] Cola llena, barco %d descartado\n", nuevoBarco->id);
        vSemaphoreDelete(nuevoBarco->sem);
        free(nuevoBarco);
        return;
    }

    // Imprimir información del barco agregado
    printf("Barco %d agregado | tipo: %s | lado: %s\n",
           nuevoBarco->id, nombreTipo(tipo), nombreLado(lado));
    printf("GUI:ADD,%d,%d,%d,%d,%d,%d\n",
           nuevoBarco->id, nuevoBarco->tipo, nuevoBarco->lado,
           nuevoBarco->prioridad, nuevoBarco->tiempoRestante, nuevoBarco->deadline);

    UBaseType_t taskPriority = (tipo == 3) ? 5 : (tipo == 2) ? 2 : 1;  // Determinar prioridad de tarea
    xTaskCreate(barcoTask, "Barco", 4096, nuevoBarco, taskPriority, NULL);  // Crear tarea del barco
}
