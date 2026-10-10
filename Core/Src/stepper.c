#include "stepper.h"
#include "corexy_config.h"
#include <math.h>
#include <stdlib.h>
#include "laser.h"

/* ===================== Estado interno ===================== */

static TIM_HandleTypeDef *s_timer;

/* posicion logica de la maquina, en mm. esto NO es necesariamente la
 * posicion fisica real hasta que no se hace homing una vez */
static volatile float s_pos_x_mm = 0.0f;
static volatile float s_pos_y_mm = 0.0f;

/* funcion que se llama (desde el main, nunca desde una isr) mientras
 * stepper_move_to espera que termine el movimiento. la usa el main para
 * seguir contestando el '?' y los avisos durante movimientos largos */
static void (*s_wait_cb)(void) = 0;

void stepper_set_wait_callback(void (*cb)(void))
{
    s_wait_cb = cb;
}

/* toda la info de un movimiento en curso. la hice volatile porque se
 * lee y escribe tanto desde el main (cuando arranca un movimiento nuevo)
 * como desde la isr del timer (que va actualizando el avance) */
typedef struct {
    int32_t steps_major_total;
    int32_t steps_minor_total;
    int32_t steps_done;
    int32_t bresenham_error;

    bool major_is_A;
    int8_t dir_A;
    int8_t dir_B;

    int32_t accel_steps;       /* largo de la rampa de entrada (0 = entra directo a velocidad de crucero) */
    int32_t decel_start_step;  /* a partir de que paso arranca a frenar */
    float   accel_rate_steps_s2;
    float   decel_rate_steps_s2;
    float   cruise_speed_steps_s;

    volatile bool move_active;
} MoveState;

static volatile MoveState s_move;

/* ===================== GPIO helpers ===================== */

/* genero un pulso de step. ojo con la polaridad: el reposo es en ALTO
 * y el pulso activo es un flanco corto a BAJO. esto es porque el driver
 * esta cableado directo a 3.3v sin resistencia, con PUL+ fijo a 3v3 y
 * PUL- conectado a este pin, asi que el opto del driver se activa
 * cuando hay diferencia de potencial (o sea, cuando este pin baja) */
static inline void motorA_step_pulse(void)
{
    HAL_GPIO_WritePin(MOTOR_A_STEP_PORT, MOTOR_A_STEP_PIN, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 20; i++) { __NOP(); }  /* ancho de pulso minimo, un par de nops alcanza */
    HAL_GPIO_WritePin(MOTOR_A_STEP_PORT, MOTOR_A_STEP_PIN, GPIO_PIN_SET);
}

static inline void motorB_step_pulse(void)
{
    HAL_GPIO_WritePin(MOTOR_B_STEP_PORT, MOTOR_B_STEP_PIN, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 20; i++) { __NOP(); }
    HAL_GPIO_WritePin(MOTOR_B_STEP_PORT, MOTOR_B_STEP_PIN, GPIO_PIN_SET);
}

static inline void set_dir_A(int8_t dir)
{
    HAL_GPIO_WritePin(MOTOR_A_DIR_PORT, MOTOR_A_DIR_PIN,
                       dir > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline void set_dir_B(int8_t dir)
{
    HAL_GPIO_WritePin(MOTOR_B_DIR_PORT, MOTOR_B_DIR_PIN,
                       dir > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ===================== Timer helper ===================== */

/* reprograma el timer para que la proxima interrupcion caiga justo en
 * period_us microsegundos. como el timer esta configurado con un tick
 * de 1us (prescaler calculado en base al clock del sistema), el periodo
 * en microsegundos se puede escribir directo en el auto reload */
static inline void schedule_next_step(float period_us)
{
    if (period_us < MIN_STEP_PERIOD_US) period_us = MIN_STEP_PERIOD_US;
    __HAL_TIM_SET_AUTORELOAD(s_timer, (uint32_t)period_us);
    __HAL_TIM_SET_COUNTER(s_timer, 0);
}

/* ===================== API pública ===================== */

void stepper_init(TIM_HandleTypeDef *step_timer)
{
    s_timer = step_timer;
    s_move.move_active = false;

    /* dejo los pines de step en reposo (alto) antes de arrancar el
     * timer, sino podria quedar un pulso raro al boot */
    HAL_GPIO_WritePin(MOTOR_A_STEP_PORT, MOTOR_A_STEP_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_B_STEP_PORT, MOTOR_B_STEP_PIN, GPIO_PIN_SET);

    HAL_TIM_Base_Start_IT(s_timer);
}

void stepper_get_position(float *x_mm, float *y_mm)
{
    *x_mm = s_pos_x_mm;
    *y_mm = s_pos_y_mm;
}

void stepper_emergency_stop(void)
{
    /* esto lo llama la interrupcion de un final de carrera inesperado.
     * con poner move_active en false alcanza: el while de abajo en
     * stepper_move_to() sale solo en la proxima vuelta */
    s_move.move_active = false;
}

void stepper_set_position(float x_mm, float y_mm)
{
    s_pos_x_mm = x_mm;
    s_pos_y_mm = y_mm;
}

void stepper_move_to(float x_mm, float y_mm, float feed_mm_min,
                      bool enter_at_cruise, bool exit_at_cruise)
{
    /* distancia a recorrer en cada eje, en mm */
    float dx_mm = x_mm - s_pos_x_mm;
    float dy_mm = y_mm - s_pos_y_mm;

    float dist_mm = sqrtf(dx_mm * dx_mm + dy_mm * dy_mm);
    if (dist_mm < 1e-4f) {
        return; /* no hay nada que mover */
    }

    /* si invertimos x por software (INVERT_X_AXIS), lo aplico solo para
     * el calculo de la cinematica, no para la posicion logica, asi la
     * maquina sigue reportando bien donde esta aunque el motor gire al
     * reves fisicamente */
    float kinematic_dx_mm = INVERT_X_AXIS ? -dx_mm : dx_mm;
    float dx_steps_f = kinematic_dx_mm * STEPS_PER_MM;
    float dy_steps_f = dy_mm * STEPS_PER_MM;

    /* ac esta la cinematica corexy posta: el motor a mueve x+y, el motor
     * b mueve x-y. por eso un movimiento diagonal perfecto (dx == dy)
     * hace que solo gire un motor y el otro se quede quieto, es como
     * funciona de verdad esta configuracion de correas */
    int32_t dA = (int32_t)lroundf(dx_steps_f + dy_steps_f);
    int32_t dB = (int32_t)lroundf(dx_steps_f - dy_steps_f);

    int32_t abs_dA = abs(dA);
    int32_t abs_dB = abs(dB);

    /* bresenham entre motor a y motor b: el que tiene mas pasos es el
     * "eje mayor" y marca el ritmo, el otro (el "menor") va intercalando
     * pasos de forma proporcional para que los dos terminen juntos */
    s_move.major_is_A = (abs_dA >= abs_dB);
    s_move.steps_major_total = s_move.major_is_A ? abs_dA : abs_dB;
    s_move.steps_minor_total = s_move.major_is_A ? abs_dB : abs_dA;
    s_move.steps_done = 0;
    s_move.bresenham_error = s_move.steps_major_total / 2;

    s_move.dir_A = (dA >= 0) ? 1 : -1;
    s_move.dir_B = (dB >= 0) ? 1 : -1;
    set_dir_A(s_move.dir_A);
    set_dir_B(s_move.dir_B);

    if (s_move.steps_major_total == 0) {
        return;
    }

    /* factor k: relaciona los pasos del eje mayor con la distancia
     * cartesiana real del segmento. hace falta porque en corexy los
     * pasos de motor no son 1 a 1 con los mm que se mueve el cabezal
     * en el plano (salvo en movimientos puramente horizontales o
     * verticales) */
    float k = (float)s_move.steps_major_total / (dist_mm * STEPS_PER_MM);

    float feed_mm_s = (feed_mm_min > 0.0f) ? (feed_mm_min / 60.0f) : DEFAULT_FEED_MM_S;
    if (feed_mm_s > MAX_FEED_MM_MIN / 60.0f) feed_mm_s = MAX_FEED_MM_MIN / 60.0f; /* tope de velocidad */
    s_move.cruise_speed_steps_s = feed_mm_s * STEPS_PER_MM * k;

    /* si enter_at_cruise es true, no hace falta rampa de entrada porque
     * ya venimos a velocidad de crucero del segmento anterior (lo decide
     * main.c comparando si este segmento sigue la misma direccion que
     * el anterior). lo mismo con exit_at_cruise para la salida */
    int32_t accel_len = enter_at_cruise ? 0 : (int32_t)lroundf(ACCEL_DISTANCE_MM * STEPS_PER_MM * k);
    int32_t decel_len = exit_at_cruise  ? 0 : (int32_t)lroundf(ACCEL_DISTANCE_MM * STEPS_PER_MM * k);

    /* si las dos rampas (cuando hacen falta las dos) no entran en lo que
     * dura el segmento, reparto el espacio disponible a la mitad entre
     * acelerar y frenar */
    if (accel_len > 0 && decel_len > 0 && (accel_len + decel_len) > s_move.steps_major_total) {
        accel_len = s_move.steps_major_total / 2;
        decel_len = s_move.steps_major_total - accel_len;
    } else if (accel_len > s_move.steps_major_total) {
        accel_len = s_move.steps_major_total;
    } else if (decel_len > s_move.steps_major_total) {
        decel_len = s_move.steps_major_total;
    }

    s_move.accel_steps = accel_len;
    s_move.decel_start_step = s_move.steps_major_total - decel_len;

    /* la formula de aceleracion sale de v^2 = 2*a*d (cinematica basica),
     * despejando a. uso tasas separadas para acelerar y frenar porque
     * pueden tener largos distintos (por ejemplo, entra a crucero pero
     * si frena al final) */
    s_move.accel_rate_steps_s2 = (accel_len > 0)
        ? (s_move.cruise_speed_steps_s * s_move.cruise_speed_steps_s) / (2.0f * accel_len)
        : 0.0f;
    s_move.decel_rate_steps_s2 = (decel_len > 0)
        ? (s_move.cruise_speed_steps_s * s_move.cruise_speed_steps_s) / (2.0f * decel_len)
        : 0.0f;

    /* actualizo la posicion logica ya, antes de que termine el
     * movimiento fisico. asumo que el movimiento se va a completar bien
     * (no hay feedback real de encoders ni nada) */
    s_pos_x_mm = x_mm;
    s_pos_y_mm = y_mm;

    s_move.move_active = true;

    /* calculo la velocidad del primer paso. si no hay rampa de entrada,
     * arranco directo a crucero */
    float v0;
    if (accel_len > 0) {
        v0 = sqrtf(2.0f * s_move.accel_rate_steps_s2 * 1.0f);
    } else {
        v0 = s_move.cruise_speed_steps_s;
    }
    if (v0 < 1.0f) v0 = 1.0f;
    /* para m4: el laser arranca con la potencia que le toca a la
     * velocidad del primer paso (en m3 esto no hace nada) */
    laser_set_speed_ratio(v0 / s_move.cruise_speed_steps_s);
    schedule_next_step(1000000.0f / v0);

    /* esto es bloqueante a proposito: me quedo esperando a que la isr
     * del timer termine el movimiento. mientras tanto, el uart sigue
     * funcionando por interrupcion asi que no se pierden bytes */
    while (s_move.move_active) {
        /* solo contesto si falta bastante para terminar el tramo: una
         * transmision dura ~3.5 ms y si el tramo termina en el medio
         * se nota un tiron entre segmentos encadenados */
        if (s_wait_cb) s_wait_cb();
        __WFI();
    }
}

/* ===================== ISR ===================== */

/* esto se llama una vez por cada "tick" del timer de pasos, osea una
 * vez por cada paso del eje mayor. ac es donde realmente se generan
 * los pulsos y se recalcula la velocidad para el proximo paso */
void stepper_timer_isr(void)
{
    if (!s_move.move_active) return;

    /* algoritmo de bresenham de toda la vida: voy restando el total del
     * eje menor, y cuando se hace negativo es porque le toca dar un paso
     * tambien, y sumo de nuevo el total del eje mayor para "resetear" */
    s_move.bresenham_error -= s_move.steps_minor_total;
    bool minor_step = false;
    if (s_move.bresenham_error < 0) {
        s_move.bresenham_error += s_move.steps_major_total;
        minor_step = true;
    }

    if (s_move.major_is_A) {
        motorA_step_pulse();
        if (minor_step) motorB_step_pulse();
    } else {
        motorB_step_pulse();
        if (minor_step) motorA_step_pulse();
    }

    s_move.steps_done++;

    if (s_move.steps_done >= s_move.steps_major_total) {
        s_move.move_active = false;
        /* si este movimiento freno, la maquina queda parada: en m4 el
         * laser tiene que quedar en 0 */
        if (s_move.decel_start_step < s_move.steps_major_total) {
            laser_set_speed_ratio(0.0f);
        }
        return;
    }

    /* perfil trapezoidal: acelerando, a velocidad constante, o frenando,
     * segun en que parte del recorrido estamos */
    float v;
    int32_t s = s_move.steps_done;

    if (s < s_move.accel_steps) {
        v = sqrtf(2.0f * s_move.accel_rate_steps_s2 * (float)s);
    } else if (s < s_move.decel_start_step) {
        v = s_move.cruise_speed_steps_s;
    } else {
        int32_t steps_remaining = s_move.steps_major_total - s;
        v = sqrtf(2.0f * s_move.decel_rate_steps_s2 * (float)steps_remaining);
    }

    if (v < 1.0f) v = 1.0f; /* para no dividir por cero mas abajo */
    /* m4: potencia proporcional a la velocidad actual / velocidad de crucero */
    laser_set_speed_ratio(v / s_move.cruise_speed_steps_s);
    schedule_next_step(1000000.0f / v);
}
