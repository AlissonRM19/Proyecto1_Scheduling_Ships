#include "strn_scheduler.h"
#include "barcos.h"

void strn_scheduler_init(strn_scheduler_t *q) {
  fcfs_scheduler_init(q);
}

bool strn_scheduler_enqueue(
  strn_scheduler_t *q,
  void *item,
  int tiempoRestante
) {
  if (q == NULL || item == NULL) {
    return false;
  }

  if (q->count >= FCFS_SCHEDULER_MAX_ITEMS) {
    return false;
  }

  int i = q->count;

  while (i > 0) {
    Barco *anterior = (Barco *) q->buffer[i - 1];

    if (anterior->tiempoRestante <= tiempoRestante) {
      break;
    }

    q->buffer[i] = q->buffer[i - 1];
    i--;
  }

  q->buffer[i] = item;
  q->count++;

  return true;
}

bool strn_scheduler_dequeue(
  strn_scheduler_t *q,
  void **item
) {
  return fcfs_scheduler_dequeue(q, item);
}