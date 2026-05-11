#include <stdlib.h>
#include <string.h>
#include "ship_pool.h"
#include "log.h"

/* ── Global definitions ──────────────────────────────────────────────── */
Barco         pool[MAX_SHIPS];
int           totalBarcos   = 0;
int           contadorId    = 1;

Cola          colaIzq, colaDer;
Barco        *barcoEnCanal  = NULL;

int           sentidoActual = 0;
int           barcosPasados = 0;
float         timerLetrero  = 0.0f;
float         simTime       = 0.0f;

TipoScheduler scheduler     = SCH_FCFS;
ModoFlujo     modo          = MODO_TICO;
int           quantum       = 2;
int           paramW        = 3;
float         tLetrero      = 5.0f;

/* ── Display helpers ─────────────────────────────────────────────────── */
const char *strTipo(int tipo) {
    if (tipo == 1) return "Normal";
    if (tipo == 2) return "Pesquero";
    if (tipo == 3) return "Patrulla";
    return "?";
}

const char *strScheduler(TipoScheduler s) {
    switch (s) {
        case SCH_FCFS:     return "FCFS";
        case SCH_PRIORITY: return "PRIORITY";
        case SCH_RR:       return "ROUND ROBIN";
        case SCH_SJF:      return "SJF";
        case SCH_STRN:     return "STRN";
        case SCH_EDF:      return "EDF";
    }
    return "?";
}

const char *strModo(ModoFlujo m) {
    if (m == MODO_TICO)    return "TICO";
    if (m == MODO_EQUIDAD) return "EQUIDAD";
    return "LETRERO";
}

/* ── Queue operations ────────────────────────────────────────────────── */
void colaInit(Cola *c) { c->count = 0; }

bool colaEnqueue(Cola *c, Barco *b) {
    if (c->count >= MAX_QUEUE) return false;
    int pos = c->count;
    for (int i = 0; i < c->count; i++) {
        bool antes = false;
        switch (scheduler) {
            case SCH_PRIORITY: antes = b->prioridad      > c->items[i]->prioridad;      break;
            case SCH_SJF:      antes = b->tiempoCruce    < c->items[i]->tiempoCruce;    break;
            case SCH_STRN:     antes = b->tiempoRestante < c->items[i]->tiempoRestante; break;
            case SCH_EDF:      antes = b->deadline       < c->items[i]->deadline;       break;
            default: break;
        }
        if (antes) { pos = i; break; }
    }
    for (int i = c->count; i > pos; i--) c->items[i] = c->items[i-1];
    c->items[pos] = b;
    c->count++;
    return true;
}

Barco *colaDequeue(Cola *c) {
    if (c->count == 0) return NULL;
    Barco *b = c->items[0];
    for (int i = 0; i < c->count - 1; i++) c->items[i] = c->items[i+1];
    c->count--;
    return b;
}

Cola *colaActual(void) { return sentidoActual == 0 ? &colaIzq : &colaDer; }
Cola *colaOtra(void)   { return sentidoActual == 0 ? &colaDer : &colaIzq; }

/* ── Pool operations ─────────────────────────────────────────────────── */
Barco *allocBarco(void) {
    for (int i = 0; i < totalBarcos; i++)
        if (!pool[i].activo) return &pool[i];
    if (totalBarcos < MAX_SHIPS) return &pool[totalBarcos++];
    return NULL;
}

Barco *findBarcoById(int id) {
    for (int i = 0; i < totalBarcos; i++)
        if (pool[i].activo && pool[i].id == id) return &pool[i];
    return NULL;
}

void removeFromQueue(Cola *c, int id) {
    for (int i = 0; i < c->count; i++) {
        if (c->items[i]->id == id) {
            for (int j = i; j < c->count - 1; j++) c->items[j] = c->items[j+1];
            c->count--;
            return;
        }
    }
}

/* ── Standalone ship creation ────────────────────────────────────────── */
void crearBarco(int tipo, int lado) {
    Barco *b = allocBarco();
    if (!b) { addLog("Pool lleno!"); return; }
    memset(b, 0, sizeof(Barco));
    b->id     = contadorId++;
    b->tipo   = tipo;
    b->lado   = lado;
    b->activo = true;
    b->estado = EST_ESPERANDO;
    switch (tipo) {
        case 1: b->tiempoCruce = 5; b->prioridad = 1; break;
        case 2: b->tiempoCruce = 4; b->prioridad = 2; break;
        case 3: b->tiempoCruce = 2; b->prioridad = 3; break;
    }
    b->tiempoRestante = b->tiempoCruce;
    b->deadline = (int)simTime + 10 + rand() % 20;
    Cola *c = (lado == 0) ? &colaIzq : &colaDer;
    if (!colaEnqueue(c, b)) {
        addLog("Cola %s llena!", lado == 0 ? "Izq" : "Der");
        b->activo = false;
        return;
    }
    addLog("Barco %d (%s, %s) -> cola", b->id, strTipo(tipo), lado == 0 ? "Izq" : "Der");
}
