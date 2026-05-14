#ifndef HARDWARE_H
#define HARDWARE_H
#include "barcos.h"
#include "freertos/semphr.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BARCOS_CANAL 5
void actualizar_tira_led();
extern Barco* barcos_activos[MAX_BARCOS_CANAL];
extern SemaphoreHandle_t canalMutex;
extern SemaphoreHandle_t flujoMutex; // Para proteger las variables de arriba


void registrarBarcoEnCanal(Barco* b);
void desregistrarBarcoEnCanal(Barco* b);

void iniciarHardware();
void leerBotones();
void mostrarTipoBarco(int tipo);

// Funciones puente para los LEDs 
void led_init();
void led_set_color(int r, int g, int b);
void led_clear();

#ifdef __cplusplus
}
#endif

#endif
