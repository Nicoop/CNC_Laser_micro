#include "laser.h"

static TIM_HandleTypeDef *s_pwm_timer;
static uint32_t s_pwm_channel;

/* todo esto se lee desde la isr del stepper y se escribe desde el main,
 * por eso es volatile */
static volatile bool     s_enabled = false;  /* m3/m4 lo habilitan, m5 lo apaga */
static volatile bool     s_dynamic = false;  /* false = m3 (constante), true = m4 (dinamico) */
static volatile bool     s_rapid   = false;  /* true durante un g0: salida forzada a 0 */
static volatile uint32_t s_prog_duty = 0;    /* s programado, ya pasado a duty (0..999) */
static volatile float    s_ratio   = 0.0f;   /* velocidad actual / velocidad de crucero */

/* pasa s (0..LASER_S_MAX) a duty. el arr de tim3 es 999 (se configura en
 * el .ioc), asi que el duty va de 0 a 999 */
static uint32_t s_to_duty(float s_value)
{
    if (s_value < 0.0f) s_value = 0.0f;
    if (s_value > LASER_S_MAX) s_value = LASER_S_MAX;
    return (uint32_t)((s_value / LASER_S_MAX) * 999.0f);
}

/* unico lugar donde se escribe el pwm. decide la salida a partir de todo
 * el estado: habilitado, g0, modo y velocidad */
static void apply(void)
{
    uint32_t duty = 0;

    if (s_enabled && !s_rapid) {
        duty = s_prog_duty;
        if (s_dynamic) {
            /* m4: la potencia sigue a la velocidad. parado = 0,
             * a velocidad de crucero = la potencia programada */
            duty = (uint32_t)((float)duty * s_ratio);
        }
    }
    __HAL_TIM_SET_COMPARE(s_pwm_timer, s_pwm_channel, duty);
}

void laser_init(TIM_HandleTypeDef *pwm_timer, uint32_t pwm_channel)
{
    s_pwm_timer = pwm_timer;
    s_pwm_channel = pwm_channel;
    s_enabled = false;
    s_dynamic = false;
    s_rapid = false;
    s_prog_duty = 0;
    s_ratio = 0.0f;

    __HAL_TIM_SET_COMPARE(s_pwm_timer, s_pwm_channel, 0);
    HAL_TIM_PWM_Start(s_pwm_timer, s_pwm_channel);
}

void laser_enable(void)
{
    /* a proposito no llamo a apply(): m3/m4 solo habilitan, la salida
     * cambia recien cuando llega un movimiento o un s */
    s_enabled = true;
}

void laser_disable(void)
{
    s_enabled = false;
    s_prog_duty = 0;
    apply();
}

void laser_set_dynamic(bool dynamic)
{
    s_dynamic = dynamic;
    /* si cambio de modo estamos sin movimiento, asi que arranco con
     * velocidad 0. tampoco llamo a apply() para no prender nada de golpe */
    s_ratio = 0.0f;
}

void laser_set_power(float s_value)
{
    s_prog_duty = s_to_duty(s_value);
    /* un s suelto llega con la maquina parada (main.c ejecuta antes el
     * movimiento pendiente), asi que el g0 anterior ya termino */
    s_rapid = false;
    apply();
}

void laser_begin_segment(bool rapid, bool has_s, float s_value)
{
    if (has_s) {
        s_prog_duty = s_to_duty(s_value);
    }
    s_rapid = rapid;
    apply(); /* una sola escritura, para no tener estados intermedios */
}

void laser_set_speed_ratio(float ratio)
{
    /* el !(ratio > 0) tambien atrapa NaN (por ejemplo 0/0) y lo deja en 0 */
    if (!(ratio > 0.0f)) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    s_ratio = ratio;

    /* en m3 no toco el hardware, asi la isr no pierde tiempo */
    if (s_dynamic) {
        apply();
    }
}

void laser_force_off_for_rapid(void)
{
    laser_begin_segment(true, false, 0.0f);
}
