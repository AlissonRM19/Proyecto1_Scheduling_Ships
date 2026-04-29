#ifndef PRIORITY_SCHEDULER_H
#define PRIORITY_SCHEDULER_H

#include <stdbool.h>

#define MAX_BARCOS_COLA 4

typedef struct {
  void* buffer[MAX_BARCOS_COLA];
  int count;
} priority_scheduler_t;

void priority_scheduler_init(priority_scheduler_t* q);

bool priority_scheduler_enqueue(
  priority_scheduler_t* q,
  void* item,
  int prioridad
);

bool priority_scheduler_dequeue(
  priority_scheduler_t* q,
  void** item
);

#endif