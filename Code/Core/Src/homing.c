#include "homing.h"
#include "corexy_config.h"
#include "z_axis.h"
#include "stepper.h"
#include "watchdog.h"

/* velocidades de busqueda, en periodo entre pasos (microsegundos).
 * cuanto mas chico el numero, mas rapido. los fui ajustando a prueba y
 * error viendo que la mecanica lo banque sin perder pasos */
#define HOMING_SEEK_PERIOD_US    200   /* primera pasada: rapida, para no perder tiempo */
#define HOMING_TOUCH_PERIOD_US   2000  /* segunda pasada: lenta, para que el toque sea preciso */

#define HOMING_BACKOFF_MM        8.0f  /* retrocede esto entre la pasada rapida y la lenta */
#define HOMING_PULLOFF_MM        4.0f  /* retrocede esto al final, antes de fijar el cero */

/* topes de seguridad por si el switch nunca se activa (cable suelto,
 * switch roto, etc). sin esto, si algo falla, el motor giraria para
 * siempre buscando un switch que nunca llega */
#define HOMING_MAX_STEPS_XY      40000
#define HOMING_MAX_STEPS_Z       80000

/* bandera para avisarle a la interrupcion de emergencia (en main.c) que
 * estamos homeando a proposito, asi no confunde un toque intencional
 * del switch con un choque real */
static volatile bool s_homing_active = false;

bool homing_is_active(void)
{
    return s_homing_active;
}

/* activo en bajo, como explique en el .h */
static inline bool x_triggered(void) { return HAL_GPIO_ReadPin(X_LIMIT_PORT, X_LIMIT_PIN) == GPIO_PIN_RESET; }
static inline bool y_triggered(void) { return HAL_GPIO_ReadPin(Y_LIMIT_PORT, Y_LIMIT_PIN) == GPIO_PIN_RESET; }
static inline bool z_triggered(void) { return HAL_GPIO_ReadPin(Z_LIMIT_PORT, Z_LIMIT_PIN) == GPIO_PIN_RESET; }

/* mismo criterio de pulso que el resto del firmware: reposo alto, pulso
 * corto a bajo */
static inline void pulse(GPIO_TypeDef *port, uint16_t pin)
{
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 20; i++) { __NOP(); }
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

/* ===== delay de microsegundos usando el contador de ciclos del cpu =====
 * HAL_Delay() solo tiene resolucion de milisegundos, que para el homing
 * me quedaba re lento. con esto (el dwt, que es un periferico de debug
 * que tiene un contador de ciclos de clock corriendo todo el tiempo)
 * puedo esperar microsegundos exactos sin usar ningun timer aparte */
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    watchdog_refresh(); /* el homing es bloqueante: sigo pateando al watchdog */
    while ((DWT->CYCCNT - start) < cycles) { }
}

void homing_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = X_LIMIT_PIN | Y_LIMIT_PIN | Z_LIMIT_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(X_LIMIT_PORT, &gpio); /* los 3 estan en el mismo puerto, GPIOB */

    dwt_init();
}

/* ===================== Z ===================== */

/* mueve z hacia el switch a una velocidad dada, hasta que se activa o
 * se llega al tope de seguridad. devuelve true si lo encontro */
static bool z_seek(uint32_t period_us, uint32_t max_steps)
{
    HAL_GPIO_WritePin(Z_DIR_PORT, Z_DIR_PIN, GPIO_PIN_SET); /* con INVERT_Z_AXIS=1, esto es la direccion negativa logica */
    for (uint32_t i = 0; i < max_steps; i++) {
        if (z_triggered()) return true;
        pulse(Z_STEP_PORT, Z_STEP_PIN);
        delay_us(period_us);
    }
    return false;
}

/* esto hace el "doble toque" tipo impresora 3d: busca rapido, retrocede
 * un poco, busca de nuevo despacio (mas preciso porque a baja velocidad
 * el error de parada es menor), y recien ahi fija la posicion como cero */
static bool home_z(void)
{
    if (!z_seek(HOMING_SEEK_PERIOD_US, HOMING_MAX_STEPS_Z)) return false;

    z_axis_set_position(0.0f);
    z_axis_move_to(HOMING_BACKOFF_MM, 0.0f); /* retrocede a velocidad normal, con rampa */

    z_axis_set_position(0.0f);
    if (!z_seek(HOMING_TOUCH_PERIOD_US, HOMING_MAX_STEPS_Z)) return false;

    z_axis_set_position(0.0f);
    z_axis_move_to(HOMING_PULLOFF_MM, 0.0f);
    z_axis_set_position(0.0f);
    return true;
}

/* ===================== X (pura: A y B van para el mismo lado) ===================== */

/* para un movimiento puramente en x (sin componente en y), en corexy
 * los dos motores tienen que girar para el mismo lado. esto lo derive
 * de la formula de la cinematica: con dy=0, dA=dx y dB=dx, osea el
 * mismo signo para los dos */
static bool x_seek(uint32_t period_us, uint32_t max_steps)
{
    HAL_GPIO_WritePin(MOTOR_A_DIR_PORT, MOTOR_A_DIR_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_B_DIR_PORT, MOTOR_B_DIR_PIN, GPIO_PIN_RESET);
    for (uint32_t i = 0; i < max_steps; i++) {
        if (x_triggered()) return true;
        pulse(MOTOR_A_STEP_PORT, MOTOR_A_STEP_PIN);
        pulse(MOTOR_B_STEP_PORT, MOTOR_B_STEP_PIN);
        delay_us(period_us);
    }
    return false;
}

static bool home_x(void)
{
    if (!x_seek(HOMING_SEEK_PERIOD_US, HOMING_MAX_STEPS_XY)) return false;

    stepper_set_position(0.0f, 0.0f);
    stepper_move_to(HOMING_BACKOFF_MM, 0.0f, 0.0f, false, false);

    stepper_set_position(0.0f, 0.0f);
    if (!x_seek(HOMING_TOUCH_PERIOD_US, HOMING_MAX_STEPS_XY)) return false;

    stepper_set_position(0.0f, 0.0f);
    stepper_move_to(HOMING_PULLOFF_MM, 0.0f, 0.0f, false, false);
    stepper_set_position(0.0f, 0.0f);
    return true;
}

/* ===================== Y (pura: A y B van para lados opuestos) ===================== */

/* para y pura (dx=0), con la misma formula dA=dy y dB=-dy, osea signos
 * opuestos entre los dos motores. tiene sentido: en corexy, mover solo
 * un motor te da una diagonal, para que el movimiento sea una linea
 * recta en y los dos motores tienen que trabajar "en contra" uno del
 * otro en la medida justa */
static bool y_seek(uint32_t period_us, uint32_t max_steps)
{
    HAL_GPIO_WritePin(MOTOR_A_DIR_PORT, MOTOR_A_DIR_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_B_DIR_PORT, MOTOR_B_DIR_PIN, GPIO_PIN_SET);
    for (uint32_t i = 0; i < max_steps; i++) {
        if (y_triggered()) return true;
        pulse(MOTOR_A_STEP_PORT, MOTOR_A_STEP_PIN);
        pulse(MOTOR_B_STEP_PORT, MOTOR_B_STEP_PIN);
        delay_us(period_us);
    }
    return false;
}

static bool home_y(void)
{
    if (!y_seek(HOMING_SEEK_PERIOD_US, HOMING_MAX_STEPS_XY)) return false;

    /* ac guardo el x actual (ya deberia estar homeado de antes) para no
     * perderlo al reescribir la posicion con stepper_set_position,
     * que pide los dos valores (x e y) juntos */
    float cur_x, cur_y;
    stepper_get_position(&cur_x, &cur_y);
    stepper_set_position(cur_x, 0.0f);
    stepper_move_to(cur_x, HOMING_BACKOFF_MM, 0.0f, false, false);

    stepper_set_position(cur_x, 0.0f);
    if (!y_seek(HOMING_TOUCH_PERIOD_US, HOMING_MAX_STEPS_XY)) return false;

    stepper_get_position(&cur_x, &cur_y);
    stepper_set_position(cur_x, 0.0f);
    stepper_move_to(cur_x, HOMING_PULLOFF_MM, 0.0f, false, false);
    stepper_get_position(&cur_x, &cur_y);
    stepper_set_position(cur_x, 0.0f);
    return true;
}

bool homing_run(void)
{
    s_homing_active = true;

    /* el orden importa: hago z primero por seguridad (alejo el cabezal
     * de la cama antes de mover x/y), despues x, despues y */
    bool ok = home_z() && home_x() && home_y();

    s_homing_active = false;
    return ok;
}
