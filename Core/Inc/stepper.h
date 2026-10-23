#ifndef STEPPER_H
#define STEPPER_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Definición de la velocidad maxima permitida de la maquina en [mm/min].
 * Si el gcode pide una <F> mas alta, "main.c" entra en alarma (igual que un final de carrera)
 * y "stepper.c" la recorta como seguro extra. */
#ifndef MAX_FEED_MM_MIN
#define MAX_FEED_MM_MIN 3000.0f
#endif

/* Se inicializan los pines para los motres X e Y (motor A y motor B del corexy)
 * Se debe llamar una vez en main(), posterior a MX_GPIO_Init() y la creación del timer.
 */

void stepper_init(TIM_HandleTypeDef *step_timer);

/* Mueve la maquina en linea recta desde donde esta hasta (x_mm, y_mm).
 * 
 * <feed_mm_min> es la velocidad en [mm/min], como pide el GCODE en el
 * parametro <f>. Esta función es bloqueante: no vuelve hasta que termino
 * el movimiento entero (por eso el "main.c" tiene que ir leyendo la UART
 * por interrupcion, sino se perderian bytes mientras se mueve).
 *
 * <enter_at_cruise> y <exit_at_cruise> son para el encadenado de movimientos
 * (el "look ahead" simplificado que arme): si el segmento anterior ya
 * iba para el mismo lado y no freno, no hace falta volver a acelerar
 * (<enter_at_cruise> = TRUE). Lo mismo al reves: si el siguiente segmento
 * sigue derecho, no frena al final (<exit_at_cruise> = TRUE). Esto es lo
 * que evita que el laser se quede pegado quemando de mas en cada
 * microsegmento del raster */
void stepper_move_to(float x_mm, float y_mm, float feed_mm_min,
                      bool enter_at_cruise, bool exit_at_cruise);

/* devuelve la posicion actual que el firmware cree que tiene (en [mm]).
 *
 * @note: ojo que esto es la posicion LOGICA, no necesariamente la fisica real
 *        (por eso hay que homear antes de confiar en esto)
 */
void stepper_get_position(float *x_mm, float *y_mm);

/* Fuerza la posicion logica sin mover nada. 
 * 
 * @note:  lo uso solo justo despues de llamar al homing, 
 *         una vez que el switch ya me dijo con certeza donde esta
 *         el cero real de la maquina
 */
void stepper_set_position(float x_mm, float y_mm);

/* Se debe llamar la función desde la ISR del timer que se configuro en
 * "stepper_init()". Acá es donde realmente se generan los pulsos de step,
 * uno por cada vez que se dispara la interrupcion
 */
void stepper_timer_isr(void);

/* @note: Corta cualquier movimiento en curso al toque. la uso desde la
 *        interrupcion de un final de carrera que se activa de improviso
 *        (no durante el homing a proposito, sino un choque real). no
 *        actualiza la posicion logica porque no se sabe bien en que paso
 *        exacto quedo cuando se corto
 */
void stepper_emergency_stop(void);

/* Registra una funcion que @function <stepper_move_to> llama una y otra vez mientras
 * espera que termine el movimiento (desde el main, no desde una ISR). El
 * main la usa para seguir contestando la consigna '?' y los avisos durante un
 * movimiento largo, asi LaserGRBL no cree que la placa se colgó. Si no
 * se registra nada, simplemente espera sin hacer nada mas */
void stepper_set_wait_callback(void (*cb)(void));

#endif /* STEPPER_H */
