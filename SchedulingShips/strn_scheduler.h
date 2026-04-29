#ifndef STRN_SCHEDULER_H
#define STRN_SCHEDULER_H

#include "fcfs_scheduler.h"

typedef fcfs_scheduler_t strn_scheduler_t;

void strn_scheduler_init(strn_scheduler_t *q);

bool strn_scheduler_enqueue(
  strn_scheduler_t *q,
  void *item,
  int tiempoRestante
);

bool strn_scheduler_dequeue(
  strn_scheduler_t *q,
  void **item
);

#endif