#ifndef COREXY_CONFIG_H
#define COREXY_CONFIG_H

#include "stm32f4xx_hal.h"

/* ======================= MOTOR / MECANICA ======================= */
/* ac explicacion basica: estas son todas las constantes que dependen de
 * como esta armada la maquina (motores, poleas, microstepping). si en
 * algun momento cambio algo de la mecanica, es aca donde hay que tocar,
 * no en el codigo de movimiento */

#define MOTOR_STEP_ANGLE_DEG    1.8f
/* angulo de paso del motor nema (lo dice el datasheet), con esto se
 * calculan los pasos por vuelta completa */
#define STEPS_PER_REV_FULL      (360.0f / MOTOR_STEP_ANGLE_DEG)   /* 200 */

#define PULLEY_TEETH            20
#define BELT_PITCH_MM           2.0f
/* cuanto avanza la correa por cada vuelta completa del motor (poleas gt2
 * de 20 dientes, paso de correa 2mm -> 40mm por vuelta) */
#define MM_PER_REV              (PULLEY_TEETH * BELT_PITCH_MM)    /* 40mm */

#define MICROSTEPPING           16      /* 8 o 16, tiene que coincidir con los dip switches del driver */

/* pasos por mm final, con todo lo de arriba ya calculado. esto es lo que
 * usa el resto del codigo para convertir mm a pasos de motor */
#define STEPS_PER_MM            ((STEPS_PER_REV_FULL * MICROSTEPPING) / MM_PER_REV)

/* ======================= PERFIL DE VELOCIDAD ===================== */
#define ACCEL_DISTANCE_MM       3.0f    /* distancia de rampa (0 -> vel. crucero) */
#define DEFAULT_FEED_MM_S       30.0f   /* velocidad de grabado por defecto, por si el gcode no manda F */
#define MIN_STEP_PERIOD_US      50      /* limite fisico de los drivers/motores a maxima velocidad, no bajar de esto */

/* invertir el sentido fisico del eje x sin tener que recablear nada.
 * lo use cuando cambie de idea sobre donde iba el final de carrera */
#define INVERT_X_AXIS           0

/* limites del area de trabajo en mm, validos recien despues de hacer el
 * homing ($H). antes de homear la posicion 0 es arbitraria asi que esto
 * no tiene sentido real todavia */
#define WORKSPACE_X_MAX_MM      240.0f
#define WORKSPACE_Y_MAX_MM      200.0f

/* ======================= PINES: MOTOR A (X+Y) ===================== */
/* en corexy no hay un motor "del eje x" y otro "del eje y" como en una
 * cartesiana comun. ac el motor a mueve la combinacion x+y, y el motor b
 * mueve x-y. eso lo hace la cinematica en stepper.c, aca solo estan los
 * pines fisicos */
#define MOTOR_A_STEP_PORT       GPIOC
#define MOTOR_A_STEP_PIN        GPIO_PIN_0
#define MOTOR_A_DIR_PORT        GPIOC
#define MOTOR_A_DIR_PIN         GPIO_PIN_1

/* ======================= PINES: MOTOR B (X-Y) ===================== */
#define MOTOR_B_STEP_PORT       GPIOC
#define MOTOR_B_STEP_PIN        GPIO_PIN_2
#define MOTOR_B_DIR_PORT        GPIOC
#define MOTOR_B_DIR_PIN         GPIO_PIN_3

#endif /* COREXY_CONFIG_H */
