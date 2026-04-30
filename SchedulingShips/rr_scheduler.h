#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reutilizamos el tamaño máximo definido para FCFS o definimos uno propio
#define RR_SCHEDULER_MAX_ITEMS 16

typedef struct {
    void *buffer[RR_SCHEDULER_MAX_ITEMS];
    int head;
    int tail;
    int count;
} rr_scheduler_t;

void rr_scheduler_init(rr_scheduler_t *q);
bool rr_scheduler_enqueue(rr_scheduler_t *q, void *item);
bool rr_scheduler_dequeue(rr_scheduler_t *q, void **item);

#ifdef __cplusplus
}
#endif

#endif