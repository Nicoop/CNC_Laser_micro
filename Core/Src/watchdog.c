#include "watchdog.h"
#include "main.h"   /* trae el hal y el extern de los handles generados por cubemx */

/* handle que genera cubemx en main.c al activar el iwdg en el .ioc */
extern IWDG_HandleTypeDef hiwdg;

bool watchdog_caused_last_reset(void)
{
    bool wd = (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET);
    __HAL_RCC_CLEAR_RESET_FLAGS();   /* limpio las banderas para el proximo arranque */
    return wd;
}

void watchdog_refresh(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}
