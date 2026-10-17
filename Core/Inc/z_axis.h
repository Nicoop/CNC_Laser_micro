#ifndef Z_AXIS_H
#define Z_AXIS_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* mecanica del eje z no tiene nada especial como las demas */
#define Z_MOTOR_STEP_ANGLE_DEG   1.8f
#define Z_STEPS_PER_REV_FULL     (360.0f / Z_MOTOR_STEP_ANGLE_DEG)  /* 200 */
#define Z_MICROSTEPPING          16
#define Z_LEAD_MM_PER_REV        8.0f   /* husillo t8x8, avanza 8mm por vuelta completa */

#define Z_STEPS_PER_MM           ((Z_STEPS_PER_REV_FULL * Z_MICROSTEPPING) / Z_LEAD_MM_PER_REV) /* 400 */

#define Z_ACCEL_DISTANCE_MM      1.0f
#define Z_DEFAULT_FEED_MM_S      3.0f
#define Z_MIN_STEP_PERIOD_US     80

/* invertir el sentido fisico de z sin recablear nada.*/
#define INVERT_Z_AXIS            1

/* limite del area de trabajo en mm, valido recien despues de hacer $H */
#define WORKSPACE_Z_MAX_MM       130.0f

/* pines step/dir del eje z  */
#define Z_STEP_PORT   GPIOC
#define Z_STEP_PIN    GPIO_PIN_4
#define Z_DIR_PORT    GPIOC
#define Z_DIR_PIN     GPIO_PIN_5

/* llamar una vez en main() */
void z_axis_init(TIM_HandleTypeDef *step_timer);

/* mueve la cama a z_mm */
void z_axis_move_to(float z_mm, float feed_mm_min);

void z_axis_get_position(float *z_mm);

/* fuerza la posicion logica sin mover nada  */
void z_axis_set_position(float z_mm);

/* llamar en la isr del timer configurado */
void z_axis_timer_isr(void);

/* corta el movimiento en curso al toque */
void z_axis_emergency_stop(void);

#endif /* Z_AXIS_H */
