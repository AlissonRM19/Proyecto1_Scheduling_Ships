#include <stddef.h>      // Requerido para NULL en C puro
#include "strn_scheduler.h"
#include "barcos.h"
#include "config.h"      // Asegúrate de que aquí esté definido STRN_SCHEDULER_MAX_ITEMS

void strn_scheduler_init(strn_scheduler_t *q) {
    q->count = 0;
    // Si tu estructura tiene un head/tail o buffer, inicialízalos aquí
    for (int i = 0; i < STRN_SCHEDULER_MAX_ITEMS; i++) {
        q->buffer[i] = NULL;
    }
}

bool strn_scheduler_enqueue(strn_scheduler_t *q, void *item, int tiempo) {
    if (q == NULL || item == NULL) return false;

    // Usar la constante correcta de STRN
    if (q->count >= STRN_SCHEDULER_MAX_ITEMS) {
        return false;
    }

    // Lógica de inserción ordenada por tiempo restante (STRN)
    int i = q->count;
    while (i > 0) {
        Barco *anterior = (Barco *) q->buffer[i - 1];
        // Si el tiempo del nuevo es menor, desplazamos el mayor a la derecha
        if (tiempo < anterior->tiempoRestante) {
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

bool strn_scheduler_dequeue(strn_scheduler_t *q, void **item) {
    if (q == NULL || q->count == 0) return false;

    *item = q->buffer[0];

    // Desplazar el resto de la cola
    for (int i = 0; i < q->count - 1; i++) {
        q->buffer[i] = q->buffer[i + 1];
    }

    q->count--;
    return true;
}
