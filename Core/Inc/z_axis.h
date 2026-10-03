#ifndef Z_AXIS_H
#define Z_AXIS_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* ===== Mecanica del eje Z ===== */
/* este eje es mas simple que x/y porque no tiene cinematica corexy, es
 * un solo motor (bah, 2 motores pero en paralelo al mismo driver asi
 * que para el firmware es como si fuera uno) moviendo un husillo */
#define Z_MOTOR_STEP_ANGLE_DEG   1.8f
#define Z_STEPS_PER_REV_FULL     (360.0f / Z_MOTOR_STEP_ANGLE_DEG)  /* 200 */
#define Z_MICROSTEPPING          16
#define Z_LEAD_MM_PER_REV        8.0f   /* husillo t8x8, avanza 8mm por vuelta completa */

#define Z_STEPS_PER_MM           ((Z_STEPS_PER_REV_FULL * Z_MICROSTEPPING) / Z_LEAD_MM_PER_REV) /* 400 */

#define Z_ACCEL_DISTANCE_MM      1.0f
#define Z_DEFAULT_FEED_MM_S      3.0f
#define Z_MIN_STEP_PERIOD_US     80

/* invertir el sentido fisico de z sin recablear nada. esto lo necesito
 * porque el switch de home de z queda arriba, y quiero que mover hacia
 * la zona de trabajo (hacia abajo) sea en positivo */
#define INVERT_Z_AXIS            1

/* limite del area de trabajo en mm, valido recien despues de hacer $H */
#define WORKSPACE_Z_MAX_MM       130.0f

/* pines step/dir del eje z (mismo conector morpho que los de x/y) */
#define Z_STEP_PORT   GPIOC
#define Z_STEP_PIN    GPIO_PIN_4
#define Z_DIR_PORT    GPIOC
#define Z_DIR_PIN     GPIO_PIN_5

/* llamar una vez en main(), despues de MX_GPIO_Init() y de crear el
 * timer handle que se va a usar para los pasos de z */
void z_axis_init(TIM_HandleTypeDef *step_timer);

/* mueve la cama a z_mm (posicion absoluta), a feed_mm_min (mm/min).
 * bloqueante, igual que stepper_move_to(). no tiene el tema de
 * enter/exit at cruise porque z normalmente no se mueve en una
 * secuencia larga de micro pasos como x/y en el raster */
void z_axis_move_to(float z_mm, float feed_mm_min);

void z_axis_get_position(float *z_mm);

/* fuerza la posicion logica sin mover nada (usar solo justo despues
 * de homing) */
void z_axis_set_position(float z_mm);

/* llamar en la isr del timer configurado en z_axis_init() */
void z_axis_timer_isr(void);

/* corta el movimiento en curso al toque, para la interrupcion de
 * emergencia de los finales de carrera */
void z_axis_emergency_stop(void);

#endif /* Z_AXIS_H */
