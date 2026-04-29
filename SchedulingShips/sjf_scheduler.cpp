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

  Barco *nuevo = (Barco*)item;

  int i = q->count;

  // mover mientras el anterior tenga mayor tiempo
  while (i > 0) {
    Barco *prev = (Barco*) q->buffer[i - 1];

    if (prev->tiempoCruce <= nuevo->tiempoCruce) {
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
