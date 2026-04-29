#ifndef FCFS_SCHEDULER_H
#define FCFS_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FCFS_SCHEDULER_MAX_ITEMS 16

typedef struct {
  void *buffer[FCFS_SCHEDULER_MAX_ITEMS];
  int head;
  int tail;
  int count;
} fcfs_scheduler_t;

void fcfs_scheduler_init(fcfs_scheduler_t *q);
bool fcfs_scheduler_enqueue(fcfs_scheduler_t *q, void *item);
bool fcfs_scheduler_dequeue(fcfs_scheduler_t *q, void **item);

#ifdef __cplusplus
}
#endif

#endif
