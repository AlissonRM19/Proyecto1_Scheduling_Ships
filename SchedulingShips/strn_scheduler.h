#ifndef STRN_SCHEDULER_H
#define STRN_SCHEDULER_H


#include <stdbool.h>
#include <stddef.h>
#include "barcos.h"
#include "config.h" // Para STRN_SCHEDULER_MAX_ITEMS

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* buffer[STRN_SCHEDULER_MAX_ITEMS];
    int count;
} strn_scheduler_t;

void strn_scheduler_init(strn_scheduler_t *q);
bool strn_scheduler_enqueue(strn_scheduler_t *q, void *item, int tiempo);
bool strn_scheduler_dequeue(strn_scheduler_t *q, void **item);

#ifdef __cplusplus
}
#endif

#endif
