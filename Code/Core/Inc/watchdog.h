#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdbool.h>

/* watchdog independiente (iwdg), configurado desde el .ioc (cubemx genera
 * hiwdg y MX_IWDG_Init, que ya lo deja arrancado). si el programa deja de
 * llamar a watchdog_refresh() por mas del tiempo configurado (~2 s), el
 * micro se reinicia solo. asi, si el firmware se cuelga con el laser
 * prendido, la placa se reinicia y el laser vuelve a apagado */

/* true si el ultimo reinicio fue causado por el watchdog. despues de
 * leerla limpia las banderas de reset para el proximo arranque */
bool watchdog_caused_last_reset(void);

/* "patear" al watchdog: reinicia la cuenta. llamar seguido, desde
 * cualquier lugar que pueda bloquear al main (esperas de movimiento,
 * homing, etc) */
void watchdog_refresh(void);

#endif /* WATCHDOG_H */
