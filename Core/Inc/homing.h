#ifndef HOMING_H
#define HOMING_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* los finales de carrera son modulos de 3 pines (vcc, gnd, señal). en
 * reposo el propio switch puentea la señal directo a 3v3, y al
 * presionarlo se corta esa conexion. por eso con el pull-down interno
 * de la nucleo activado, quedan activos en BAJO: reposo = alto (switch
 * haciendo de puente a vcc), presionado = bajo (se corta el puente y el
 * pull-down tira la señal para abajo) */
#define X_LIMIT_PORT   GPIOB
#define X_LIMIT_PIN    GPIO_PIN_0
#define Y_LIMIT_PORT   GPIOB
#define Y_LIMIT_PIN    GPIO_PIN_1
#define Z_LIMIT_PORT   GPIOB
#define Z_LIMIT_PIN    GPIO_PIN_2

/* llamar una vez en main(), antes de usar homing_run() */
void homing_init(void);

/* corre el ciclo completo de homing: primero z, despues x, despues y.
 * es bloqueante (tarda unos segundos en terminar). devuelve true si los
 * 3 ejes homearon bien, false si algun switch nunca se activo (podria
 * ser un cable suelto, un switch roto, o que la maquina se trabo antes
 * de llegar) */
bool homing_run(void);

/* devuelve true mientras homing_run() esta corriendo. esto es
 * importante: la interrupcion de los finales de carrera (la de
 * emergencia, en main.c) tiene que IGNORAR los triggers mientras esto
 * es true, porque durante el homing tocar el switch a proposito es
 * justamente lo que se espera que pase, no es un choque */
bool homing_is_active(void);

#endif /* HOMING_H */
