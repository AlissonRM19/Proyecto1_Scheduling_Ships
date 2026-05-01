#include "rr_scheduler.h"
#include "fcfs_scheduler.h"

// Inicializa la cola de Round Robin
void rr_scheduler_init(rr_scheduler_t *q) {
    // Reutilizamos la inicialización de FCFS, ya que RR es una extensión de FCFS
    fcfs_scheduler_init((fcfs_scheduler_t *)q); 
}

// Encola un barco en la cola de Round Robin
bool rr_scheduler_enqueue(rr_scheduler_t *q, void *item) {
    return fcfs_scheduler_enqueue((fcfs_scheduler_t *)q, item);
}

// Desencola un barco de la cola de Round Robin
bool rr_scheduler_dequeue(rr_scheduler_t *q, void **item) {
    return fcfs_scheduler_dequeue((fcfs_scheduler_t *)q, item);
}
