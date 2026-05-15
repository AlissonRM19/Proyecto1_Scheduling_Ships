#pragma once
#include "types.h"

/* ── App-wide state (defined in ship_pool.c) ─────────────────────────── */
extern Barco         pool[MAX_SHIPS];
extern int           totalBarcos;
extern int           contadorId;

extern Cola          colaIzq, colaDer;
extern Barco        *barcoEnCanal;

extern int           sentidoActual;
extern int           barcosPasados;
extern float         timerLetrero;
extern float         simTime;

extern TipoScheduler scheduler;
extern ModoFlujo     modo;
extern int           quantum;
extern int           paramW;
extern float         tLetrero;

/* ── Queue operations ────────────────────────────────────────────────── */
void   colaInit(Cola *c);
bool   colaEnqueue(Cola *c, Barco *b);
Barco *colaDequeue(Cola *c);
Cola  *colaActual(void);
Cola  *colaOtra(void);

/* ── Pool operations ─────────────────────────────────────────────────── */
Barco *allocBarco(void);
Barco *findBarcoById(int id);
void   removeFromQueue(Cola *c, int id);

/* ── Standalone ship creation ────────────────────────────────────────── */
void crearBarco(int tipo, int lado);

/* ── Display helpers ─────────────────────────────────────────────────── */
const char *strTipo(int tipo);
const char *strScheduler(TipoScheduler s);
const char *strModo(ModoFlujo m);
