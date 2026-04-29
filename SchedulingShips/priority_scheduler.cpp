#include <cstddef>
#include "priority_scheduler.h"
#include "barcos.h"

void priority_scheduler_init(priority_scheduler_t* q) {
  if (q == NULL) return;

  q->count = 0;
}

bool priority_scheduler_enqueue(
  priority_scheduler_t* q,
  void* item,
  int prioridad
) {
  if (q == NULL || item == NULL) return false;

  if (q->count >= MAX_BARCOS_COLA) {
    return false;
  }

  int i = q->count;

  while (i > 0 && ((Barco*)q->buffer[i - 1])->prioridad < prioridad) {
    void* anterior = q->buffer[i - 1];

    // Esto luego lo resolvemos desde barcos.h
    // por ahora solo movemos espacio
    q->buffer[i] = q->buffer[i - 1];
    i--;
  }

  q->buffer[i] = item;
  q->count++;

  return true;
}

bool priority_scheduler_dequeue(
  priority_scheduler_t* q,
  void** item
) {
  if (q == NULL || item == NULL) return false;

  if (q->count == 0) {
    *item = NULL;
    return false;
  }

  *item = q->buffer[0];

  for (int i = 0; i < q->count - 1; i++) {
    q->buffer[i] = q->buffer[i + 1];
  }

  q->count--;

  return true;
}
