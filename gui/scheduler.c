#include "scheduler.h"
#include "ship_pool.h"
#include "serial.h"
#include "log.h"

void schedulerUpdate(float dt) {
    simTime += dt;

    /* MODO_LETRERO: switch direction on a fixed timer (standalone only) */
    if (!connected && modo == MODO_LETRERO) {
        timerLetrero += dt;
        if (timerLetrero >= tLetrero) {
            timerLetrero = 0.0f;
            sentidoActual = 1 - sentidoActual;
            barcosPasados = 0;
            addLog("[Letrero] Cambio -> %s", sentidoActual == 0 ? "Izq" : "Der");
        }
    }

    /* Animate the ship currently crossing the canal */
    if (barcoEnCanal != NULL) {
        barcoEnCanal->tiempoEnSlice += dt;
        float fracBase = (float)(barcoEnCanal->tiempoCruce - barcoEnCanal->tiempoRestante)
                         / (float)barcoEnCanal->tiempoCruce;
        float fracSlice = (barcoEnCanal->tiempoEnSlice / (float)barcoEnCanal->sliceActual)
                          * ((float)barcoEnCanal->sliceActual / (float)barcoEnCanal->tiempoCruce);
        barcoEnCanal->progreso = fracBase + fracSlice;
        if (barcoEnCanal->progreso > 1.0f) barcoEnCanal->progreso = 1.0f;
        float left  = (float)CANAL_X;
        float right = (float)(CANAL_X + CANAL_W - SHIP_W);
        if (barcoEnCanal->lado == 0)
            barcoEnCanal->posX = left + (right - left) * barcoEnCanal->progreso;
        else
            barcoEnCanal->posX = right - (right - left) * barcoEnCanal->progreso;
        barcoEnCanal->posY = (float)CANAL_MID_Y;

        /* Slice expiry — standalone only; connected mode waits for DONE/RETURN */
        if (!connected && barcoEnCanal->tiempoEnSlice >= (float)barcoEnCanal->sliceActual) {
            barcoEnCanal->tiempoRestante -= barcoEnCanal->sliceActual;
            bool preemptive = (scheduler == SCH_RR || scheduler == SCH_STRN);
            if (preemptive && barcoEnCanal->tiempoRestante > 0) {
                addLog("%s: Barco %d regresa (resta %ds)",
                    strScheduler(scheduler), barcoEnCanal->id, barcoEnCanal->tiempoRestante);
                barcoEnCanal->estado = EST_ESPERANDO;
                Cola *c = (barcoEnCanal->lado == 0) ? &colaIzq : &colaDer;
                colaEnqueue(c, barcoEnCanal);
                barcosPasados++;
            } else {
                addLog("Barco %d termino!", barcoEnCanal->id);
                barcoEnCanal->estado = EST_TERMINADO;
                barcoEnCanal->activo = false;
                barcosPasados++;
            }
            barcoEnCanal = NULL;
        }
        return;
    }

    /* Dispatch — standalone only */
    if (connected) return;
    if (colaIzq.count == 0 && colaDer.count == 0) return;

    /* Direction switching logic per mode */
    if (modo == MODO_TICO) {
        if (colaActual()->count == 0 && colaOtra()->count > 0) {
            sentidoActual = 1 - sentidoActual;
            addLog("[Tico] Cambiando a %s", sentidoActual == 0 ? "Izq" : "Der");
        }
    } else if (modo == MODO_EQUIDAD) {
        if (barcosPasados >= paramW && colaOtra()->count > 0) {
            sentidoActual = 1 - sentidoActual;
            barcosPasados = 0;
            addLog("[Equidad] W=%d, cambiando a %s", paramW, sentidoActual == 0 ? "Izq" : "Der");
        } else if (colaActual()->count == 0 && colaOtra()->count > 0) {
            sentidoActual = 1 - sentidoActual;
            barcosPasados = 0;
        }
    } else { /* MODO_LETRERO */
        if (colaActual()->count == 0 && colaOtra()->count > 0) {
            sentidoActual = 1 - sentidoActual;
            barcosPasados = 0;
        }
    }

    if (colaActual()->count == 0) return;
    Barco *next = colaDequeue(colaActual());
    if (!next) return;

    next->estado = EST_CRUZANDO;
    next->tiempoEnSlice = 0.0f;
    int slice = next->tiempoRestante;
    if (scheduler == SCH_RR && slice > quantum) slice = quantum;
    next->sliceActual = slice;

    float fracBase = (float)(next->tiempoCruce - next->tiempoRestante) / (float)next->tiempoCruce;
    float left  = (float)CANAL_X;
    float right = (float)(CANAL_X + CANAL_W - SHIP_W);
    next->posX = (next->lado == 0) ? left  + (right - left) * fracBase
                                   : right - (right - left) * fracBase;
    next->posY = (float)CANAL_MID_Y;
    barcoEnCanal = next;
    addLog("%s -> Barco %d (%s) cruza %ds",
        strScheduler(scheduler), next->id, strTipo(next->tipo), slice);
}
