#include "laser.h"

static TIM_HandleTypeDef *s_pwm_timer;
static uint32_t s_pwm_channel;
static bool s_enabled = false;

void laser_init(TIM_HandleTypeDef *pwm_timer, uint32_t pwm_channel)
{
    s_pwm_timer = pwm_timer;
    s_pwm_channel = pwm_channel;
    s_enabled = false;

    /* arranca en 0 por las dudas, y ahi si arranco el pwm en hardware */
    __HAL_TIM_SET_COMPARE(s_pwm_timer, s_pwm_channel, 0);
    HAL_TIM_PWM_Start(s_pwm_timer, s_pwm_channel);
}

void laser_enable(void)
{
    s_enabled = true;
}

void laser_disable(void)
{
    s_enabled = false;
    __HAL_TIM_SET_COMPARE(s_pwm_timer, s_pwm_channel, 0);
}

void laser_set_power(float s_value)
{
    if (!s_enabled) {
        __HAL_TIM_SET_COMPARE(s_pwm_timer, s_pwm_channel, 0);
        return;
    }

    if (s_value < 0.0f) s_value = 0.0f;
    if (s_value > LASER_S_MAX) s_value = LASER_S_MAX;

    /* el arr de tim3 lo configuramos en 999 desde el .ioc (eso define
     * el periodo del pwm), asi que el duty va de 0 a 999 proporcional
     * a s/s_max. por ejemplo s=500 de 1000 maximo da duty=499, osea
     * mas o menos 50% */
    uint32_t duty = (uint32_t)((s_value / LASER_S_MAX) * 999.0f);
    __HAL_TIM_SET_COMPARE(s_pwm_timer, s_pwm_channel, duty);
}

void laser_force_off_for_rapid(void)
{
    /* a proposito NO toco s_enabled aca: esto es un apagado momentaneo
     * para el traslado (g0), no es lo mismo que un m5 real. despues de
     * este g0, si viene un g1 con s, tiene que prender de nuevo normal
     * sin que yo tenga que mandar m3 otra vez */
    __HAL_TIM_SET_COMPARE(s_pwm_timer, s_pwm_channel, 0);
}
