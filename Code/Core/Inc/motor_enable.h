#ifndef MOTOR_ENABLE_H
#define MOTOR_ENABLE_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* un solo pin, atado al ena- de los 3 drivers (x, y, z) todos en
 * paralelo. el ena+ de los 3 va a 3v3, igual que step/dir. tuve que
 * probar la polaridad a mano girando los motores para confirmarla:
 * alto (set) = motores enganchados, bajo (reset) = motores libres
 * (se pueden mover a mano) */
#define MOTOR_ENA_PORT   GPIOC
#define MOTOR_ENA_PIN    GPIO_PIN_7

void motor_enable_init(void);
void motors_enable(void);   /* responde al m17 */
void motors_disable(void);  /* responde al m84 */

/* true si los motores estan activos (enganchados) en este momento */
bool motors_are_enabled(void);

#endif /* MOTOR_ENABLE_H */
