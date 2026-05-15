#include "strn_scheduler.h"  // Incluir STRN scheduler
#include <stddef.h>  // Incluir size_t

// Inicializa la cola de Shortest Remaining Time Next (STRN)
void strn_scheduler_init(strn_scheduler_t *q) {
    if (q == NULL) return; // Validación de seguridad  // Verificar puntero nulo
    
    // Inicializamos el contador y el buffer de la cola
    q->count = 0;  // Contador a 0
    for (int i = 0; i < STRN_SCHEDULER_MAX_ITEMS; i++) {
        q->buffer[i] = NULL;  // Poner NULL
    }
}

// Encola un barco en la posición correcta según su tiempo restante (STRN)
bool strn_scheduler_enqueue(strn_scheduler_t *q, void *item, int tiempo) {
    if (q == NULL || item == NULL) return false;  // Verificar punteros

    // Usar la constante STRN_SCHEDULER_MAX_ITEMS para verificar el límite de la cola
    if (q->count >= STRN_SCHEDULER_MAX_ITEMS) {
        return false;  // Cola llena
    }

    // Lógica de inserción ordenada por tiempo restante (STRN)
    int i = q->count;  // Índice de inserción
    while (i > 0) {
        Barco *anterior = (Barco *) q->buffer[i - 1];  // Cast a Barco
        // Si el tiempo del nuevo es menor, desplazamos el mayor a la derecha
        if (tiempo < anterior->tiempoRestante) {
            q->buffer[i] = q->buffer[i - 1];  // Mover elemento
            i--;  // Decrementar índice
        } else {
            break;  // Salir
        }
    }

    // Insertamos el nuevo barco en la posición correcta
    q->buffer[i] = item;  // Insertar item
    q->count++;  // Incrementar contador
    return true;  // Éxito
}

// Desencola un barco de la cola de STRN
bool strn_scheduler_dequeue(strn_scheduler_t *q, void **item) {
    if (q == NULL || q->count == 0) return false;  // Verificar punteros y si vacía

    *item = q->buffer[0];  // Obtener primer item

    // Desplazar el resto de la cola
    for (int i = 0; i < q->count - 1; i++) {
        q->buffer[i] = q->buffer[i + 1];  // Mover elementos
    }

    // Limpiamos la última posición y decrementamos el contador
    q->count--;  // Decrementar contador
    return true;  // Éxito
}
