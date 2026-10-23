#ifndef HOMING_H
#define HOMING_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Los finales de carrera son modulos de 3 pines (VCC, GND, SGNL). 
 * 
 * En reposo, el propio switch puentea la señal directo a 3V3, y al
 * presionarlo se corta esa conexion. Por eso con el pull-down interno
 * de la nucleo activado, quedan activos en BAJO: 
 * - reposo = alto (switch haciendo de puente a VCC)
 * - presionado = bajo (se corta el puente y el pull-down tira la señal para abajo)
 * 
*/

#define X_LIMIT_PORT   GPIOB
#define X_LIMIT_PIN    GPIO_PIN_0
#define Y_LIMIT_PORT   GPIOB
#define Y_LIMIT_PIN    GPIO_PIN_1
#define Z_LIMIT_PORT   GPIOB
#define Z_LIMIT_PIN    GPIO_PIN_2

/* Se debe llamar una vez en main(), antes de usar homing_run() */
void homing_init(void);

/* Realiza la rutina de homing completa: primero z, despues x, despues y.
 * Es de naturaleza bloqueante (tarda unos segundos en terminar). 
 * Sevuelve true si los 3 ejes realizaron el homing de forma exitosa,
 * false si algun switch nunca se activo (e.g. un cable suelto, un switch roto,
 *  o que la maquina se trabo antes de llegar)
 */
 bool homing_run(void);

/* Devuelve true mientras homing_run() esta corriendo. 
 * 
 * Este flag tiene una razón de ser: la interrupción de los finales de carrera (la de
 * emergencia, en main.c) tiene que IGNORAR los triggers mientras el flag = TRUE, 
 * porque durante el homing se espera que se activen los switches, sabiendo que
 * la causa no fue un choque, sino la misma rutina de homing.
 */
bool homing_is_active(void);

#endif /* HOMING_H */
