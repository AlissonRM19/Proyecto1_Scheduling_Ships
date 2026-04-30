#include "rr_scheduler.h"
#include "fcfs_scheduler.h"

void rr_scheduler_init(rr_scheduler_t *q) {
    // Al incluir fcfs_scheduler.h, el compilador ya sabe qué es esto
    fcfs_scheduler_init((fcfs_scheduler_t *)q); 
}

bool rr_scheduler_enqueue(rr_scheduler_t *q, void *item) {
    return fcfs_scheduler_enqueue((fcfs_scheduler_t *)q, item);
}

bool rr_scheduler_dequeue(rr_scheduler_t *q, void **item) {
    return fcfs_scheduler_dequeue((fcfs_scheduler_t *)q, item);
}
