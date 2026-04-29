#ifndef SJF_SCHEDULER_H
#define SJF_SCHEDULER_H

#include <stdbool.h>
#include "fcfs_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef fcfs_scheduler_t sjf_scheduler_t;

void sjf_scheduler_init(sjf_scheduler_t *q);
bool sjf_scheduler_enqueue(sjf_scheduler_t *q, void *item, int tiempoCruce);
bool sjf_scheduler_dequeue(sjf_scheduler_t *q, void **item);

#ifdef __cplusplus
}
#endif

#endif
