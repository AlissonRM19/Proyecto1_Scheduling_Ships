#ifndef EDF_SCHEDULER_H
#define EDF_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDF_SCHEDULER_MAX_ITEMS 16

typedef struct {
    void* buffer[EDF_SCHEDULER_MAX_ITEMS];
    int count;
} edf_scheduler_t;

void edf_scheduler_init(edf_scheduler_t* q);
bool edf_scheduler_enqueue(edf_scheduler_t* q, void* item, int deadline);
bool edf_scheduler_dequeue(edf_scheduler_t* q, void** item);

#ifdef __cplusplus
}
#endif

#endif