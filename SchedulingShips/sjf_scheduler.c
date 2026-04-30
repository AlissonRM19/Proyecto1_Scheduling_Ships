#include <stddef.h>
#include <stdbool.h>
#include "sjf_scheduler.h"
#include "barcos.h"

void sjf_scheduler_init(sjf_scheduler_t* q) {
    if (q == NULL) return;
    q->count = 0;
    for (int i = 0; i < SJF_SCHEDULER_MAX_ITEMS; i++) {
        q->buffer[i] = NULL;
    }
}

// Cambiado Barco* a void* para coincidir con el header
bool sjf_scheduler_enqueue(sjf_scheduler_t* q, void* item, int tiempoCruce) {
    if (q == NULL || item == NULL) return false;
    if (q->count >= SJF_SCHEDULER_MAX_ITEMS) return false;

    int i = q->count;

    while (i > 0) {
        // Aquí hacemos el cast para poder leer .tiempoCruce
        Barco* anterior = (Barco*)q->buffer[i - 1];
        
        if (tiempoCruce < anterior->tiempoCruce) {
            q->buffer[i] = q->buffer[i - 1];
            i--;
        } else {
            break;
        }
    }

    q->buffer[i] = item;
    q->count++;
    return true;
}

bool sjf_scheduler_dequeue(sjf_scheduler_t* q, void** item) {
    if (q == NULL || item == NULL) return false;
    if (q->count == 0) return false;

    *item = q->buffer[0];

    for (int i = 0; i < q->count - 1; i++) {
        q->buffer[i] = q->buffer[i + 1];
    }

    q->count--;
    return true;
}
