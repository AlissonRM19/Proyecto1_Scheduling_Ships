#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H

#include <stdbool.h>
#include "fcfs_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef fcfs_scheduler_t rr_scheduler_t;

void rr_scheduler_init(rr_scheduler_t *q);
bool rr_scheduler_enqueue(rr_scheduler_t *q, void *item);
bool rr_scheduler_dequeue(rr_scheduler_t *q, void **item);

#ifdef __cplusplus
}
#endif

#endif
