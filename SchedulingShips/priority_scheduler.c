#include <stddef.h>
#include "priority_scheduler.h"
#include "barcos.h" // Necesario para castear void* a Barco*

// Inicializa la cola de prioridad
void priority_scheduler_init(priority_scheduler_t* q) {
    if (q == NULL) return;
    q->count = 0;
    for (int i = 0; i < MAX_BARCOS_COLA; i++) {
        q->buffer[i] = NULL;
    }
}

// Encola un barco en la posición correcta según su prioridad
bool priority_scheduler_enqueue(priority_scheduler_t* q, Barco* item, int prioridad) {
    if (q == NULL || item == NULL) return false;

    // Verificamos si hay espacio en la cola
    if (q->count >= MAX_BARCOS_COLA) {
        return false;
    }

    // Buscamos la posición correcta (Inserción Ordenada)
    int i = q->count;
    
    // Mientras no estemos al inicio Y la prioridad del nuevo sea MAYOR a la del anterior
    // (Asumiendo que números más altos significan más prioridad, ej: Patrulla = 3)
    while (i > 0) {
        Barco* anterior = (Barco*)q->buffer[i - 1];
        
        if (prioridad > anterior->prioridad) {
            // Desplazamos el barco con menor prioridad a la derecha
            q->buffer[i] = q->buffer[i - 1];
            i--;
        } else {
            // Encontramos su lugar
            break;
        }
    }

    // Insertamos el nuevo barco en la posición correcta
    // item representa el nuevo barco a encolar, y prioridad es su prioridad
    // q es la cola de prioridad donde se va a insertar el barco
    // q es de tipo priority_scheduler_t, que contiene un buffer de void* y un contador de elementos
    q->buffer[i] = item;
    q->count++;
    return true;
}

bool priority_scheduler_dequeue(priority_scheduler_t* q, void** item) {
    if (q == NULL || item == NULL) return false;

    if (q->count == 0) {
        *item = NULL;
        return false;
    }

    // El de mayor prioridad siempre está en la posición 0
    *item = q->buffer[0];

    // Desplazamos todos los demás una posición a la izquierda
    for (int i = 0; i < q->count - 1; i++) {
        q->buffer[i] = q->buffer[i + 1];
    }

    q->count--;
    return true;
}