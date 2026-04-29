#include "edf_scheduler.h"
#include "barcos.h"

void edf_scheduler_init(edf_scheduler_t *q) {
  q->count = 0;
}

bool edf_scheduler_enqueue(edf_scheduler_t *q, void *item, int deadline) {
  if (q == NULL || item == NULL) {
    return false;
  }

  if (q->count >= EDF_SCHEDULER_MAX_ITEMS) {
    return false;
  }

  Barco *nuevo = (Barco*)item;

  int i = q->count;

  // Insertar ordenado por deadline ascendente (menor deadline primero)
  while (i > 0) {
    Barco *prev = (Barco*) q->buffer[i - 1];

    if (prev->deadline <= nuevo->deadline) {
      break;
    }

    q->buffer[i] = q->buffer[i - 1];
    i--;
  }

  q->buffer[i] = item;
  q->count++;

  return true;
}

bool edf_scheduler_dequeue(edf_scheduler_t *q, void **item) {
  if (q == NULL || item == NULL || q->count == 0) {
    return false;
  }

  *item = q->buffer[0];

  // Shift left
  for (int i = 1; i < q->count; i++) {
    q->buffer[i - 1] = q->buffer[i];
  }

  q->count--;

  return true;
}