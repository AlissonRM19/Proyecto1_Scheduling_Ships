#include "fcfs_scheduler.h"

void fcfs_scheduler_init(fcfs_scheduler_t *q) {
  if (q == NULL) {
    return;
  }
  q->head = 0;
  q->tail = 0;
  q->count = 0;
}

bool fcfs_scheduler_enqueue(fcfs_scheduler_t *q, void *item) {
  if (q == NULL || item == NULL) {
    return false;
  }
  if (q->count >= FCFS_SCHEDULER_MAX_ITEMS) {
    return false;
  }
  q->buffer[q->tail] = item;
  q->tail = (q->tail + 1) % FCFS_SCHEDULER_MAX_ITEMS;
  q->count++;
  return true;
}

bool fcfs_scheduler_dequeue(fcfs_scheduler_t *q, void **item) {
  if (q == NULL || item == NULL) {
    return false;
  }
  if (q->count == 0) {
    *item = NULL;
    return false;
  }
  *item = q->buffer[q->head];
  q->head = (q->head + 1) % FCFS_SCHEDULER_MAX_ITEMS;
  q->count--;
  return true;
}
