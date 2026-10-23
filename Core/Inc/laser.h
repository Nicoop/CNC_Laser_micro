#ifndef LASER_H
#define LASER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Variable que determina la potencia maxima que acepta el GCODE en el campo <s>
 * Tiene que coincidir con lo que se le informa a LaserGRLB en $30 (ver el $$ de <main.c>) 
 */
#define LASER_S_MAX   1000.0f

/* Se debe llamar una vez en main(), despues de <MX_TIM3_Init()> */
void laser_init(TIM_HandleTypeDef *pwm_timer, uint32_t pwm_channel);

/* Habilita el laser (Comandos <M3>/<M4>). 
 * Solo se "destraba" la función de utilizar el laser: la salida la define la
 * potencia programada <s> y, en <M4>, la velocidad del cabezal. 
 */

void laser_enable(void);

/* Se encarga de apagar el laser con <M5>: salida a 0 y setea la potencia programada a 0
 * Eso implica que despues de un <M5> hace falta mandar un s nuevo
 * (Es más conservador que un GRBL real, el cual retiene el último valor de <s> y lo vuelve a usar en el próximo <M3>/<M4>)
 */

void laser_disable(void);

/* Permite 2 modos de operación:
 * false -> <M3>: La potencia es constante.
 * true -> <M4>: La potencia es dinámica (la salida se escala con la velocidad actual / velocidad de crucero)
 */

void laser_set_dynamic(bool dynamic);

/* Setea/fija la potencia programada (<s> suelto, sin movimiento) */
void laser_set_power(float s_value);

/* Debe llamarse antes de ejecutar cualquier segmento de movimiento. Los elementos de entrada son:
 * @rapid: es un G0 (la salida va a 0 mientras dure).
 * @has_s / @s_value = la linea contiene un nuevo valor de <s> (la variable <s> es modal, si no
 * se tiene un nuevo valor de <s> se utiliza el último valor seteado) 
 */

void laser_begin_segment(bool rapid, bool has_s, float s_value);

/* @velocidad actual / @velocidad de crucero (0..1). Invoca a la ISR del
 * stepper en cada paso. En <M3> no hace nada con el hardware
 */

void laser_set_speed_ratio(float ratio);

/* Función necesaria para compatibilidad.
 * Es equivale a laser_begin_segment(true,false,0)
 */
void laser_force_off_for_rapid(void);

#endif /* LASER_H */
