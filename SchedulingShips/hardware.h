#ifndef HARDWARE_H
#define HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif

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
