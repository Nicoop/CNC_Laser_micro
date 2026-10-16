#ifndef LASER_H
#define LASER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* potencia maxima que acepta el gcode en el campo s. tiene que coincidir
 * con lo que se le informa a laser grbl en $30 (ver el $$ de main.c) */
#define LASER_S_MAX   1000.0f

/* llamar una vez en main(), despues de MX_TIM3_Init() */
void laser_init(TIM_HandleTypeDef *pwm_timer, uint32_t pwm_channel);

/* habilita el laser (m3/m4). solo "destraba": la salida la define la
 * potencia programada (s) y, en m4, la velocidad del cabezal */
void laser_enable(void);

/* apaga el laser (m5): salida a 0 y borra la potencia programada, asi
 * despues de un m5 hace falta mandar un s nuevo para volver a quemar
 * (mas conservador que un grbl real, que se acuerda del ultimo s) */
void laser_disable(void);

/* false = m3, potencia constante. true = m4, potencia dinamica: la
 * salida se escala con la velocidad actual / velocidad de crucero */
void laser_set_dynamic(bool dynamic);

/* fija la potencia programada (s suelto, sin movimiento) */
void laser_set_power(float s_value);

/* se llama justo antes de ejecutar cada segmento de movimiento.
 * rapid = es un g0 (la salida va a 0 mientras dure).
 * has_s / s_value = la linea traia un s nuevo (el s es modal, si no
 * viene se sigue usando el ultimo) */
void laser_begin_segment(bool rapid, bool has_s, float s_value);

/* velocidad actual / velocidad de crucero (0..1). la llama la isr del
 * stepper en cada paso. en m3 no hace nada con el hardware */
void laser_set_speed_ratio(float ratio);

/* se deja por compatibilidad: equivale a laser_begin_segment(true,false,0) */
void laser_force_off_for_rapid(void);

#endif /* LASER_H */
