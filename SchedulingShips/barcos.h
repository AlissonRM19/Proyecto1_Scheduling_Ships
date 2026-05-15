#ifndef BARCOS_H
#define BARCOS_H

#ifdef __cplusplus
extern "C" {
#endif

// Tus estructuras (ejemplo)
typedef struct {
    int id;
    int tipo;
    int lado;
    int posicion;
    int tiempoCruce;
    int tiempoRestante;
    int prioridad;
    int deadline;
    void* sem; 
} Barco;

// Declaraciones de funciones
void crearBarco(int tipo, int lado);
void barcoTask(void *pvParameters);


#ifdef __cplusplus
}
#endif

#endif
