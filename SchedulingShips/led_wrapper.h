#ifndef LED_WRAPPER_H  // Guardia de inclusión
#define LED_WRAPPER_H

#ifdef __cplusplus
extern "C" {  // Declarar extern C
#endif

void led_init();  // Inicializar LEDs
void led_clear();  // Limpiar LEDs
void led_set_pixel(int index, int r, int g, int b);  // Establecer pixel
void led_show();  // Mostrar LEDs

#ifdef __cplusplus
}
#endif

#endif