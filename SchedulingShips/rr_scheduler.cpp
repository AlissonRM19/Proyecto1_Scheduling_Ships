#include "rr_scheduler.h"

void rr_scheduler_init(rr_scheduler_t *q) {
  fcfs_scheduler_init(q);
}

bool rr_scheduler_enqueue(rr_scheduler_t *q, void *item) {
  return fcfs_scheduler_enqueue(q, item);
}

bool rr_scheduler_dequeue(rr_scheduler_t *q, void **item) {
  return fcfs_scheduler_dequeue(q, item);
}
