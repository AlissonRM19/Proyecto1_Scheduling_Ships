#ifndef PRIORITY_SCHEDULER_H
#define PRIORITY_SCHEDULER_H

#include <stdbool.h>
#include "barcos.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BARCOS_COLA 4

typedef struct {
  void* buffer[MAX_BARCOS_COLA];
  int count;
} priority_scheduler_t;

void priority_scheduler_init(priority_scheduler_t *s);
bool priority_scheduler_enqueue(priority_scheduler_t *s, Barco *b, int p);
bool priority_scheduler_dequeue(priority_scheduler_t *s, void **b);

#ifdef __cplusplus
}
#endif

#endif
