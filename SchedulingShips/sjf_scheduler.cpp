#include "sjf_scheduler.h"
#include "barcos.h"

void sjf_scheduler_init(sjf_scheduler_t *q) {
  fcfs_scheduler_init(q);
}

bool sjf_scheduler_enqueue(sjf_scheduler_t *q, void *item, int tiempoCruce) {
  if (q == NULL || item == NULL) {
    return false;
  }

  if (q->count >= FCFS_SCHEDULER_MAX_ITEMS) {
    return false;
  }

  int i = q->count;

  while (i > 0) {
    Barco *anterior = (Barco *) q->buffer[i - 1];
    if (anterior->tiempoCruce <= tiempoCruce) {
      break;
    }
    q->buffer[i] = q->buffer[i - 1];
    i--;
  }

  q->buffer[i] = item;
  q->count++;

  return true;
}

bool sjf_scheduler_dequeue(sjf_scheduler_t *q, void **item) {
  return fcfs_scheduler_dequeue(q, item);
}
