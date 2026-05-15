#ifndef HARDWARE_H  // Guardia de inclusión
#define HARDWARE_H

#ifdef __cplusplus
extern "C" {  // Declarar extern C
#endif

void iniciarHardware();  // Inicializar hardware
void leerBotones();  // Leer botones
void mostrarTipoBarco(int tipo);  // Mostrar tipo de barco

// Funciones puente para los LEDs
void led_init();  // Inicializar LEDs
void led_set_color(int r, int g, int b);  // Establecer color LED
void led_clear();  // Limpiar LEDs

#ifdef __cplusplus
}
#endif

#endif
