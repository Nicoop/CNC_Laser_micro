#ifndef LASER_H
#define LASER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* potencia maxima que acepta el gcode en el campo s. laser grbl por
 * defecto asume 1000 en configuraciones modernas, si la version que
 * usan es mas vieja puede ser 0-255, en ese caso cambiar este numero */
#define LASER_S_MAX   1000.0f

/* llamar una vez en main(), despues de MX_TIM3_Init() */
void laser_init(TIM_HandleTypeDef *pwm_timer, uint32_t pwm_channel);

/* habilita el laser (esto responde a m3/m4). ojo que esto solo
 * "destraba" el sistema, no prende nada por si solo: la potencia real
 * la pone laser_set_power(). si nunca se llama a esto, cualquier s que
 * llegue se ignora */
void laser_enable(void);

/* apaga el laser (m5) y lleva el duty a 0 de una, sin esperar nada */
void laser_disable(void);

/* fija la potencia entre 0 y LASER_S_MAX. si el laser esta deshabilitado
 * (porque se llamo a laser_disable antes) esto no hace nada hasta el
 * proximo laser_enable() */
void laser_set_power(float s_value);

/* apaga la salida DE UNA, sin tocar el estado de habilitado ni la
 * potencia que tenia guardada. la uso antes de cada movimiento rapido
 * (g0): en un grbl real, en modo laser, se apaga el laser en todo g0
 * pase lo que pase con el s de esa linea, por seguridad. despues de
 * esto, el proximo s que llegue vuelve a aplicar normal si el laser
 * sigue habilitado */
void laser_force_off_for_rapid(void);

#endif /* LASER_H */
