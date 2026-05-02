#ifndef LED_WRAPPER_H
#define LED_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

void led_init();
void led_clear();
void led_set_pixel(int index, int r, int g, int b);
void led_show();

#ifdef __cplusplus
}
#endif

#endif