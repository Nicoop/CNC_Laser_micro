#include "motor_enable.h"

static bool s_enabled = false;

void motor_enable_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = MOTOR_ENA_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOTOR_ENA_PORT, &gpio);

    motors_enable(); /* arrancan enganchados por defecto al bootear */
}

void motors_enable(void)
{
    HAL_GPIO_WritePin(MOTOR_ENA_PORT, MOTOR_ENA_PIN, GPIO_PIN_SET);
    s_enabled = true;
}

void motors_disable(void)
{
    HAL_GPIO_WritePin(MOTOR_ENA_PORT, MOTOR_ENA_PIN, GPIO_PIN_RESET);
    s_enabled = false;
}

bool motors_are_enabled(void)
{
    return s_enabled;
}
