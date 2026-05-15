#ifndef BARCOS_H  // Guardia de inclusión para evitar múltiples inclusiones
#define BARCOS_H  // Definir la macro de guardia

#ifdef __cplusplus  // Si se compila en C++, declarar como extern "C"
extern "C" {
#endif

// Definición de la estructura Barco
typedef struct {
    int id;  // Identificador único del barco
    int tipo;  // Tipo del barco (1: Normal, 2: Pesquero, 3: Patrulla)
    int lado;  // Lado desde donde viene (0: Izquierda, 1: Derecha)
    int posicion;  // Posición actual en el canal (número de LED)
    int tiempoCruce;  // Tiempo total necesario para cruzar el canal
    int tiempoRestante;  // Tiempo restante para completar el cruce
    int prioridad;  // Prioridad del barco para el planificador
    int deadline;  // Deadline para el planificador EDF
    void* sem;  // Semáforo asociado al barco
} Barco;

// Declaraciones de funciones
void crearBarco(int tipo, int lado);  // Función para crear un nuevo barco
void barcoTask(void *pvParameters);  // Tarea principal ejecutada por cada barco

#ifdef __cplusplus
}
#endif

#endif
