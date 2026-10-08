#ifndef STEPPER_H
#define STEPPER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* este modulo maneja los motores de x e y (motor a y motor b del corexy).
 * llamar una vez en main(), despues de MX_GPIO_Init() y de crear el
 * timer que se va a usar para generar los pulsos de step */
void stepper_init(TIM_HandleTypeDef *step_timer);

/* mueve la maquina en linea recta desde donde esta hasta (x_mm, y_mm).
 * feed_mm_min es la velocidad en mm/min, como pide el gcode en el
 * parametro f. esta funcion es bloqueante: no vuelve hasta que termino
 * el movimiento entero (por eso el main.c tiene que ir leyendo el uart
 * por interrupcion, sino se perderian bytes mientras se mueve)
 *
 * enter_at_cruise y exit_at_cruise son para el encadenado de movimientos
 * (el "look ahead" simplificado que arme): si el segmento anterior ya
 * iba para el mismo lado y no freno, no hace falta volver a acelerar
 * (enter_at_cruise = true). lo mismo al reves: si el siguiente segmento
 * sigue derecho, no frena al final (exit_at_cruise = true). esto es lo
 * que evita que el laser se quede pegado quemando de mas en cada
 * microsegmento del raster */
void stepper_move_to(float x_mm, float y_mm, float feed_mm_min,
                      bool enter_at_cruise, bool exit_at_cruise);

/* devuelve la posicion actual que el firmware cree que tiene (en mm).
 * ojo que esto es la posicion LOGICA, no necesariamente la fisica real
 * (por eso hay que homear antes de confiar en esto) */
void stepper_get_position(float *x_mm, float *y_mm);

/* fuerza la posicion logica sin mover nada. lo uso solo justo despues
 * de homing, una vez que el switch ya me dijo con certeza donde esta
 * el cero real de la maquina */
void stepper_set_position(float x_mm, float y_mm);

/* hay que llamar a esto desde la isr del timer que se configuro en
 * stepper_init(). ac es donde realmente se generan los pulsos de step,
 * uno por cada vez que se dispara la interrupcion */
void stepper_timer_isr(void);

/* corta cualquier movimiento en curso al toque. la uso desde la
 * interrupcion de un final de carrera que se activa de improviso
 * (no durante el homing a proposito, sino un choque real). no
 * actualiza la posicion logica porque no se sabe bien en que paso
 * exacto quedo cuando se corto */
void stepper_emergency_stop(void);

#endif /* STEPPER_H */
