#ifndef MOTOR_ENABLE_H
#define MOTOR_ENABLE_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Se utiliza un solo pin, atado al +ENA de los 3 drivers (x, y, z) todos en
 * PARALELO. El -ENA de los 3 drivers se conecta a 3v3, igual que step/dir. 
 *
 * tuve que probar la polaridad a mano girando los motores para confirmarla:
 * -> alto (set) = motores enganchados
 * -> bajo (reset) = motores libres (se pueden mover a mano) 
 */
 
#define MOTOR_ENA_PORT   GPIOC
#define MOTOR_ENA_PIN    GPIO_PIN_7

void motor_enable_init(void);
void motors_enable(void);           /* responde al <M17> */
void motors_disable(void);          /* responde al <M84> */

/* Devuelve TRUE si los motores estan activos (enganchados) en el instante
 * en el que se llama a la función.
 */
bool motors_are_enabled(void);

#endif /* MOTOR_ENABLE_H */
