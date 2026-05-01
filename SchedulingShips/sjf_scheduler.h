#ifndef SJF_SCHEDULER_H
#define SJF_SCHEDULER_H

#include <stdbool.h>
#include "barcos.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* buffer[SJF_SCHEDULER_MAX_ITEMS];
    int count;
} sjf_scheduler_t;

void sjf_scheduler_init(sjf_scheduler_t *q);
// Usamos void* item para que coincida con el estándar de los otros schedulers
bool sjf_scheduler_enqueue(sjf_scheduler_t *q, void *item, int tiempoCruce);
bool sjf_scheduler_dequeue(sjf_scheduler_t *q, void **item);

#ifdef __cplusplus
}
#endif

#endif
