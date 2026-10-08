#include "z_axis.h"
#include <math.h>
#include <stdlib.h>

static TIM_HandleTypeDef *s_timer;
static volatile float s_pos_z_mm = 0.0f;

/* mismo esquema que stepper.c pero mas simple: un solo motor, no hace
 * falta bresenham ni nada de eso */
typedef struct {
    int32_t steps_total;
    int32_t steps_done;
    int8_t  dir;

    int32_t accel_steps;
    int32_t decel_start_step;
    float   accel_steps_per_s2;
    float   cruise_speed_steps_s;

    volatile bool move_active;
} ZMoveState;

static volatile ZMoveState s_move;

/* mismo criterio de pulso que stepper.c: reposo en alto, pulso corto a
 * bajo (cableado directo a 3.3v sin resistencia) */
static inline void z_step_pulse(void)
{
    HAL_GPIO_WritePin(Z_STEP_PORT, Z_STEP_PIN, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 20; i++) { __NOP(); }
    HAL_GPIO_WritePin(Z_STEP_PORT, Z_STEP_PIN, GPIO_PIN_SET);
}

static inline void z_set_dir(int8_t dir)
{
    HAL_GPIO_WritePin(Z_DIR_PORT, Z_DIR_PIN, dir > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline void z_schedule_next_step(float period_us)
{
    if (period_us < Z_MIN_STEP_PERIOD_US) period_us = Z_MIN_STEP_PERIOD_US;
    __HAL_TIM_SET_AUTORELOAD(s_timer, (uint32_t)period_us);
    __HAL_TIM_SET_COUNTER(s_timer, 0);
}

void z_axis_init(TIM_HandleTypeDef *step_timer)
{
    s_timer = step_timer;
    s_move.move_active = false;

    HAL_GPIO_WritePin(Z_STEP_PORT, Z_STEP_PIN, GPIO_PIN_SET);

    HAL_TIM_Base_Start_IT(s_timer);
}

void z_axis_get_position(float *z_mm)
{
    *z_mm = s_pos_z_mm;
}

void z_axis_emergency_stop(void)
{
    s_move.move_active = false;
}

void z_axis_set_position(float z_mm)
{
    s_pos_z_mm = z_mm;
}

void z_axis_move_to(float z_mm, float feed_mm_min)
{
    float dz_mm = z_mm - s_pos_z_mm;
    float dist_mm = fabsf(dz_mm);
    if (dist_mm < 1e-4f) {
        return;
    }

    int32_t steps_total = (int32_t)lroundf(dist_mm * Z_STEPS_PER_MM);
    if (steps_total == 0) {
        return;
    }

    s_move.steps_total = steps_total;
    s_move.steps_done = 0;
    s_move.dir = (dz_mm >= 0.0f) ? 1 : -1;
    /* aca aplico la inversion de z. ojo que la aplico sobre la direccion
     * fisica nomas, la posicion logica (s_pos_z_mm) se actualiza normal
     * mas abajo, sin inversion */
    if (INVERT_Z_AXIS) s_move.dir = -s_move.dir;
    z_set_dir(s_move.dir);

    float feed_mm_s = (feed_mm_min > 0.0f) ? (feed_mm_min / 60.0f) : Z_DEFAULT_FEED_MM_S;
    s_move.cruise_speed_steps_s = feed_mm_s * Z_STEPS_PER_MM;

    /* rampa simetrica de toda la vida: si no entra completa en el
     * recorrido, la recorto a la mitad para cada lado */
    int32_t accel_steps = (int32_t)lroundf(Z_ACCEL_DISTANCE_MM * Z_STEPS_PER_MM);
    if (accel_steps * 2 > steps_total) {
        accel_steps = steps_total / 2;
    }
    s_move.accel_steps = accel_steps;
    s_move.decel_start_step = steps_total - accel_steps;

    if (accel_steps > 0) {
        s_move.accel_steps_per_s2 =
            (s_move.cruise_speed_steps_s * s_move.cruise_speed_steps_s) / (2.0f * accel_steps);
    } else {
        s_move.accel_steps_per_s2 = 0.0f;
    }

    s_pos_z_mm = z_mm;
    s_move.move_active = true;

    float v0 = sqrtf(2.0f * s_move.accel_steps_per_s2 * 1.0f);
    if (v0 < 1.0f) v0 = 1.0f;
    z_schedule_next_step(1000000.0f / v0);

    /* bloqueante, igual que stepper_move_to() */
    while (s_move.move_active) {
        __WFI();
    }
}

void z_axis_timer_isr(void)
{
    if (!s_move.move_active) return;

    z_step_pulse();
    s_move.steps_done++;

    if (s_move.steps_done >= s_move.steps_total) {
        s_move.move_active = false;
        return;
    }

    float v;
    int32_t s = s_move.steps_done;

    if (s < s_move.accel_steps) {
        v = sqrtf(2.0f * s_move.accel_steps_per_s2 * (float)s);
    } else if (s < s_move.decel_start_step) {
        v = s_move.cruise_speed_steps_s;
    } else {
        int32_t remaining = s_move.steps_total - s;
        v = sqrtf(2.0f * s_move.accel_steps_per_s2 * (float)remaining);
    }

    if (v < 1.0f) v = 1.0f;
    z_schedule_next_step(1000000.0f / v);
}
