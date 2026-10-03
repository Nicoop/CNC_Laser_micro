/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* todos mis modulos propios. cada uno maneja una parte de la maquina
 * por separado (motores x/y, eje z, laser, enable de motores, homing,
 * parser de gcode), asi el main.c queda mas que nada como el que
 * conecta todo y maneja la comunicacion */
#include "corexy_config.h"
#include "homing.h"
#include "motor_enable.h"
#include "z_axis.h"
#include "laser.h"
#include "gcode_parser.h"
#include "stepper.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* bandera de alarma: se activa cuando un final de carrera se dispara
 * de improviso en medio de un movimiento normal (no durante el homing
 * a proposito). mientras esta activa, el firmware rechaza cualquier
 * comando nuevo hasta que se mande $X a mano. asi me aseguro de que si
 * la maquina choco contra algo, no siga como si nada mientras laser
 * grbl le sigue mandando lineas del trabajo */
static volatile bool alarm_active = false;
/* esta otra bandera es para que el mensaje de alarma se mande una sola
 * vez (desde el main, nunca desde la interrupcion) aunque el switch
 * rebote mecanicamente y dispare la interrupcion varias veces por una
 * sola pulsada real */
static volatile bool alarm_message_pending = false;

/* esto es el "look ahead" simplificado: en vez de ejecutar cada linea
 * de gcode apenas llega, guardo el movimiento pendiente y espero a ver
 * la linea siguiente antes de decidir si freno al final o no. si el
 * siguiente movimiento sigue exactamente la misma direccion (mismo
 * signo de avance en x e y), no hace falta frenar entre uno y otro -
 * eso es lo que evita que el laser se quede pegado quemando de mas en
 * cada microsegmento del raster */
typedef struct {
    bool valid;
    float target_x, target_y, feed;
    float dir_x, dir_y;   /* signo normalizado del movimiento: -1, 0, +1 */
} PendingMove;
static PendingMove g_pending = {0};
static uint32_t g_last_line_tick = 0;
/* si no llega una linea nueva en este tiempo, asumo que no hay
 * continuacion y ejecuto el movimiento pendiente frenando normal (por
 * ejemplo, si justo esa era la ultima linea del archivo) */
#define LOOKAHEAD_TIMEOUT_MS 80

/* banderas para mandar mensajes desde el main en vez de desde las
 * interrupciones. esto es clave: si se manda un HAL_UART_Transmit()
 * directo desde una isr, puede chocar con una transmision que ya este
 * en curso desde el main, y el uart queda trabado para siempre. por
 * eso la isr SOLO prende una bandera, y el unico que transmite de
 * verdad es el main() */
static volatile uint8_t welcome_pending = 0;
static volatile uint8_t status_report_pending = 0;

/* buffer circular para los bytes que van llegando por uart. hace falta
 * porque mientras un movimiento esta en curso (stepper_move_to es
 * bloqueante) el main no puede ir leyendo el uart activamente, pero la
 * interrupcion de recepcion si sigue funcionando y puede ir guardando
 * los bytes nuevos aca hasta que el main este libre para procesarlos */
#define RX_RING_SIZE 256
static volatile uint8_t  rx_ring[RX_RING_SIZE];
static volatile uint16_t rx_ring_head = 0;
static volatile uint16_t rx_ring_tail = 0;

#define GCODE_LINE_BUF_SIZE 96
static uint8_t g_absolute_mode = 1; /* 1 = G90 (absoluto, por defecto en Grbl), 0 = G91 (relativo) */
static GcodeCommandType g_last_motion = GCODE_G1;
static float g_last_feed_mm_min = 0.0f; /* el f es modal: si una linea no lo trae, se sigue usando el ultimo que llego */
static uint8_t  rx_byte;
static char     line_buf[GCODE_LINE_BUF_SIZE];
static uint16_t line_idx = 0;
static volatile uint8_t line_ready = 0; /* esta ya no se usa para nada (quedo de una version vieja), pero no molesta */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */
static void flush_pending_move(bool exit_at_cruise);
static void send_status_report(UART_HandleTypeDef *huart);
static void process_gcode_line(char *raw_line);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  /* inicializo todos mis modulos. el orden ac no importa mucho entre
   * ellos, lo que si importa es que esto pase despues de que el cubemx
   * ya configuro los gpio y los timers arriba */
  homing_init();
  motor_enable_init();
  z_axis_init(&htim4);
  stepper_init(&htim2);
  laser_init(&htim3, TIM_CHANNEL_1);
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1); /* arranco la recepcion por interrupcion, un byte a la vez */

  /* mando el mensaje de bienvenida tipo grbl apenas arranca, para el
   * caso de que algun programa ya este escuchando el puerto desde antes
   * de que yo conecte el cable */
  const char *welcome = "\r\nGrbl 1.1h ['$' for help]\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*)welcome, strlen(welcome), 100);
  stepper_init(&htim2); /* esto quedo duplicado de una version anterior, no hace falta pero tampoco molesta */
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /* reviso las banderas de mensajes pendientes ANTES que nada mas,
	   * asi salen apenas se puede, no dependen de que termine de
	   * procesar otra cosa primero */
	  if (alarm_message_pending) {
	      alarm_message_pending = false;
	      const char *alarm = "ALARM: Final de carrera activado durante el movimiento\r\n";
	      HAL_UART_Transmit(&huart2, (uint8_t*)alarm, strlen(alarm), 100);
	  }
	  if (welcome_pending) {
	      welcome_pending = 0;
	      const char *welcome = "\r\nGrbl 1.1h ['$' for help]\r\n";
	      HAL_UART_Transmit(&huart2, (uint8_t*)welcome, strlen(welcome), 100);
	  }
      if (status_report_pending) {
          status_report_pending = 0;
          send_status_report(&huart2);
      }

      /* si quedo un movimiento pendiente esperando a ver si la proxima
       * linea continua en la misma direccion, y paso demasiado tiempo
       * sin que llegue nada nuevo, lo ejecuto igual frenando normal.
       * esto es importante para que no se quede "trabado" esperando
       * para siempre el ultimo segmento de un trabajo */
      if (g_pending.valid && (HAL_GetTick() - g_last_line_tick > LOOKAHEAD_TIMEOUT_MS)) {
          flush_pending_move(false); /* frena normal, no llegó continuación */
      }

      /* vacio el buffer circular armando lineas completas de texto.
       * cuando encuentro un \n o \r, ahi se termino una linea y la
       * mando a procesar */
      while (rx_ring_tail != rx_ring_head) {
          uint8_t b = rx_ring[rx_ring_tail];
          rx_ring_tail = (rx_ring_tail + 1) % RX_RING_SIZE;

          if (b == '\n' || b == '\r') {
              if (line_idx > 0) {
                  line_buf[line_idx] = '\0';
                  process_gcode_line(line_buf);
                  line_idx = 0;

                  /* Chequeamos el '?' pendiente DESPUÉS de cada línea,
                   * no solo una vez por vuelta del while externo, porque
                   * este bucle interno puede seguir de largo procesando
                   * muchas líneas sin volver arriba mientras dura un trabajo. */
                  if (status_report_pending) {
                      status_report_pending = 0;
                      send_status_report(&huart2);
                  }
              }
          } else if (line_idx < GCODE_LINE_BUF_SIZE - 1) {
              line_buf[line_idx++] = (char)b;
          }
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  /* esto es todo generado por el cubemx, no lo toque. configura el
   * clock del sistema a 84mhz usando el pll a partir del oscilador
   * interno (hsi) */
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  /* tim2 es el timer que uso para generar los pasos de x/y (stepper.c).
   * el prescaler=83 sobre un clock de apb1 de 84mhz da un tick de
   * exactamente 1us, que es la unidad que uso en todo el codigo de
   * movimiento para calcular los periodos entre pasos */

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  /* tim3 genera el pwm del laser (canal 1, pin pc6). period=999 define
   * la frecuencia del pwm (con prescaler=83 sobre 84mhz, cada tick es
   * 1us, entonces 999+1 ticks de periodo = 1khz de pwm, que esta
   * dentro de lo que acepta el modulo laser). el duty cycle (0 a 999)
   * es lo que controla laser.c segun la potencia s que venga del gcode */

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{
  /* tim4 es el timer de pasos del eje z (mismo criterio que tim2 pero
   * para z_axis.c) */

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 83;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  /* usart2 (pa2/pa3) es el puerto que usa la nucleo para salir por el
   * mismo cable usb del st-link como puerto com virtual. por eso no
   * hace falta ningun adaptador externo para hablar con la pc */

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 */
  /* estos 3 son los finales de carrera de x, y, z. los configure como
   * interrupcion externa con flanco descendente (no como entrada
   * comun) para que cualquier toque de un switch en medio de un
   * movimiento normal dispare al toque la parada de emergencia, sin
   * depender de que el codigo este en ese momento revisando el pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* ac configuro a mano los pines de step/dir de los motores a y b
   * (x/y) y del eje z, todos en el puerto c. los dejo como salida
   * push-pull normal, nada de especial, porque los manejo directo con
   * HAL_GPIO_WritePin desde stepper.c/z_axis.c, no uso ningun
   * periferico de hardware para esto (son gpio comunes, el timing lo
   * hace software con el timer de pasos) */
  GPIO_InitTypeDef motor_gpio = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE(); /* ya está habilitado más arriba, pero no molesta repetirlo */

  motor_gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
  motor_gpio.Mode = GPIO_MODE_OUTPUT_PP;
  motor_gpio.Pull = GPIO_NOPULL;
  motor_gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &motor_gpio);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* convierte un float a texto con 3 decimales fijos, sin usar printf con
 * %f (que en este toolchain embebido no siempre anda bien con floats
 * sin configuracion extra, asi que lo hago a mano con enteros) */
static void format_float3(float value, char *out, size_t out_size)
{
    int sign = (value < 0.0f) ? -1 : 1;
    float absval = value * sign;
    int32_t scaled = (int32_t)lroundf(absval * 1000.0f);
    int32_t whole = scaled / 1000;
    int32_t frac  = scaled % 1000;
    snprintf(out, out_size, "%s%ld.%03ld", (sign < 0) ? "-" : "", (long)whole, (long)frac);
}

/* arma y manda el reporte de estado que laser grbl pide con el
 * caracter '?'. el formato "<Idle|MPos:x,y,z|FS:f,s>" es el que usa
 * grbl de verdad, lo imito para que laser grbl lo entienda y pueda
 * mostrar la posicion actual en su interfaz. por ahora mando z fijo en
 * 0.000 porque no lo estoy reportando todavia (se podria agregar
 * despues si hace falta) */
static void send_status_report(UART_HandleTypeDef *huart)
{
    float x, y;
    stepper_get_position(&x, &y);
    char xs[16], ys[16], msg[64];
    format_float3(x, xs, sizeof(xs));
    format_float3(y, ys, sizeof(ys));
    int len = snprintf(msg, sizeof(msg), "<Idle|MPos:%s,%s,0.000|FS:0,0>\r\n", xs, ys);
    HAL_UART_Transmit(huart, (uint8_t*)msg, len, 100);
}

/* ejecuta el movimiento que quedo "pendiente" esperando a ver si el
 * siguiente seguia la misma direccion. exit_at_cruise le dice a
 * stepper_move_to si tiene que frenar al final o no (eso ya se decidio
 * afuera, comparando con la linea que acaba de llegar).
 *
 * uso una variable static (last_entered_at_cruise) para acordarme como
 * salio el movimiento ANTERIOR, porque eso define como entra este: si
 * el anterior no freno (salio a velocidad de crucero), este tiene que
 * entrar directo a crucero tambien, sin perder tiempo acelerando de
 * nuevo de cero */
static void flush_pending_move(bool exit_at_cruise)
{
    if (!g_pending.valid) return;
    static bool last_entered_at_cruise = false;
    motors_enable(); /* por si estaban desactivados (m84), los reactivo antes de intentar mover */
    stepper_move_to(g_pending.target_x, g_pending.target_y, g_pending.feed,
                     last_entered_at_cruise, exit_at_cruise);
    last_entered_at_cruise = exit_at_cruise; /* el próximo entra como éste salió */
    g_pending.valid = false;
}

/* esta es la funcion mas importante de todo el firmware: recibe una
 * linea de texto ya armada (sin \r\n) y decide que hacer con ella. se
 * llama una vez por cada linea de gcode que llega completa desde el
 * buffer circular */
static void process_gcode_line(char *raw_line)
{
    char *gcode_str = raw_line;
    uint8_t is_jog = 0;

    /* $X es el comando de grbl para desbloquear la alarma. lo chequeo
     * primero que nada, antes incluso de ver si hay una alarma activa,
     * porque si no nunca se podria salir de la alarma */
    if (raw_line[0] == '$' && raw_line[1] == 'X' && raw_line[2] == '\0') {
        alarm_active = false;
        HAL_UART_Transmit(&huart2, (uint8_t*)"ok\r\n", 4, 100);
        return;
    }

    /* si esta en alarma (por un final de carrera que se activo de
     * improviso), rechazo cualquier otra cosa que llegue hasta que
     * manden $X. ni siquiera llego a parsear la linea */
    if (alarm_active) {
        const char *err = "ALARM: Máquina bloqueada por un final de carrera. Mandá $X para desbloquear.\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t*)err, strlen(err), 100);
        return;
    }

    /* comandos que arrancan con $ son comandos especiales de grbl, no
     * gcode normal */
    if (raw_line[0] == '$') {
        if (raw_line[1] == 'J' && raw_line[2] == '=') {
            /* $J=... es un comando de jog (los botones de flecha de
             * laser grbl mandan esto). le saco el prefijo y lo trato
             * como una linea de gcode normal de ahi en mas */
            gcode_str = &raw_line[3];
            is_jog = 1;
        } else if (raw_line[1] == 'H' && raw_line[2] == '\0') {
            /* $H es homing */
            bool ok = homing_run();
            if (ok) {
                /* homing_run ya deja todo en 0 internamente, pero lo
                 * vuelvo a fijar ac por las dudas, no cuesta nada */
                stepper_set_position(0.0f, 0.0f);
                z_axis_set_position(0.0f);
                HAL_UART_Transmit(&huart2, (uint8_t*)"ok\r\n", 4, 100);
            } else {
                const char *err = "ALARM: Homing falló (algún switch nunca se activó)\r\n";
                HAL_UART_Transmit(&huart2, (uint8_t*)err, strlen(err), 100);
            }
            return;
        } else {
            /* otros comandos $ (tipo $$, $G, etc) por ahora solo los
             * confirmo sin hacer nada, para que laser grbl no se quede
             * esperando una respuesta que nunca llega */
            HAL_UART_Transmit(&huart2, (uint8_t*)"ok\r\n", 4, 100);
            return;
        }
    }

    GcodeCommand cmd = {0};
    if (gcode_parse_line(gcode_str, &cmd)) {

    	/* si esta linea NO es una continuacion de movimiento (viene un
    	 * g0, un cambio de z, apagar el laser, o desactivar motores),
    	 * primero tengo que ejecutar lo que haya quedado pendiente del
    	 * encadenado de segmentos. sino ese ultimo tramo se perderia o
    	 * se mezclaria mal con esta linea nueva */
    	if (cmd.type == GCODE_G0 || cmd.has_z || cmd.has_m5 || cmd.has_m84) {
    	    flush_pending_move(false);
    	}

        if (cmd.has_g90) g_absolute_mode = 1;
        if (cmd.has_g91) g_absolute_mode = 0;
        if (cmd.has_m3 || cmd.has_m4) {
            laser_enable();
        }
        if (cmd.has_m84) {
            motors_disable();
        }
        if (cmd.has_m17) {
            motors_enable();
        }
        if (cmd.has_m5) {
            laser_disable();
        }
        if (cmd.has_s) {
            laser_set_power(cmd.s);
        }
        uint8_t explicit_motion = (cmd.type == GCODE_G0 || cmd.type == GCODE_G1);
        if (explicit_motion) g_last_motion = cmd.type;

        /* en todo g0 apago el laser de una, sin importar si la linea
         * trae s o no. esto imita lo que hace un grbl real en modo
         * laser: nunca se quiere que el laser quede prendido durante
         * un traslado rapido, es una cuestion de seguridad */
        if (cmd.type == GCODE_G0) {
            laser_force_off_for_rapid();
        }

        /* should_move me dice si esta linea trae alguna coordenada de
         * x o y (gcode modal: puede venir sin g0/g1 explicito y aun
         * asi ser un movimiento valido, eso ya lo resuelve el parser) */
        uint8_t should_move = (cmd.has_x || cmd.has_y);
        (void)g_last_motion; /* reservado para cuando distingamos velocidad G0 vs G1 */

        /* si hay que moverse pero los motores estan desactivados (m84),
         * aviso y no ejecuto nada. este chequeo quedo duplicado dos
         * veces seguidas de cuando lo fui armando, no hace falta pero
         * tampoco rompe nada dejarlo asi */
        if (should_move && !motors_are_enabled()) {
            const char *warn = "WARNING: Motores desactivados (M84). Mandá M17 para reactivar.\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)warn, strlen(warn), 100);
            should_move = 0; /* no ejecutamos el movimiento */
        }
        if (should_move && !motors_are_enabled()) {
            const char *warn = "WARNING: Motores desactivados (M84). Mandá M17 para reactivar.\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)warn, strlen(warn), 100);
            should_move = 0;
        }

        if (should_move) {
            float cur_x, cur_y;
            stepper_get_position(&cur_x, &cur_y);
            if (g_pending.valid) {
                /* si ya hay un movimiento pendiente en cola, calculo
                 * esta nueva linea a partir de DONDE VA A TERMINAR ese
                 * pendiente, no de donde esta la maquina ahora mismo
                 * (que todavia no se movio fisicamente) */
                cur_x = g_pending.target_x;
                cur_y = g_pending.target_y;
            }

            float target_x, target_y;
            if (g_absolute_mode) {
                target_x = cmd.has_x ? cmd.x : cur_x;
                target_y = cmd.has_y ? cmd.y : cur_y;
            } else {
                target_x = cur_x + (cmd.has_x ? cmd.x : 0.0f);
                target_y = cur_y + (cmd.has_y ? cmd.y : 0.0f);
            }

            /* limite de software del area de trabajo. esto solo tiene
             * sentido real despues de haber hecho $H, porque antes de
             * eso la posicion 0 es arbitraria y no corresponde a
             * ninguna esquina fisica de verdad */
            if (target_x < 0.0f || target_x > WORKSPACE_X_MAX_MM ||
                target_y < 0.0f || target_y > WORKSPACE_Y_MAX_MM) {
                const char *err = "ALARM: Movimiento fuera del área de trabajo\r\n";
                HAL_UART_Transmit(&huart2, (uint8_t*)err, strlen(err), 100);
            } else {
                if (cmd.has_f) {
                    g_last_feed_mm_min = cmd.f;
                }
                float feed = g_last_feed_mm_min;

                /* calculo la direccion normalizada (un vector unitario)
                 * de este movimiento, para despues poder comparar con
                 * el siguiente y ver si siguen exactamente la misma
                 * direccion */
                float new_dx = target_x - cur_x;
                float new_dy = target_y - cur_y;
                float new_len = sqrtf(new_dx*new_dx + new_dy*new_dy);
                float new_dir_x = (new_len > 1e-4f) ? (new_dx / new_len) : 0.0f;
                float new_dir_y = (new_len > 1e-4f) ? (new_dy / new_len) : 0.0f;

                if (g_pending.valid) {
                    /* producto punto entre las dos direcciones: si da
                     * practicamente 1, significa que apuntan para el
                     * mismo lado exacto (colineales), asi que no hace
                     * falta frenar entre uno y otro */
                    float dot = g_pending.dir_x * new_dir_x + g_pending.dir_y * new_dir_y;
                    bool continues = (dot > 0.999f);
                    flush_pending_move(continues);
                }

                /* esta linea queda como la nueva pendiente, todavia no
                 * se ejecuta: se ejecuta recien cuando llegue la
                 * proxima (ahi se decide si frena o no) o si pasa
                 * mucho tiempo sin que llegue nada mas (ver el timeout
                 * en el while principal) */
                g_pending.valid = true;
                g_pending.target_x = target_x;
                g_pending.target_y = target_y;
                g_pending.feed = feed;
                g_pending.dir_x = new_dir_x;
                g_pending.dir_y = new_dir_y;
                g_last_line_tick = HAL_GetTick();
            }
        }

        /* el eje z se maneja aparte, no entra en el encadenado de
         * segmentos de x/y (normalmente se usa para ajustar el foco
         * antes de grabar, no en medio de un trazo) */
        if (cmd.has_z) {
            if (!motors_are_enabled()) {
                const char *warn = "WARNING: Motores desactivados (M84). Mandá M17 para reactivar.\r\n";
                HAL_UART_Transmit(&huart2, (uint8_t*)warn, strlen(warn), 100);
            } else {
                float target_z;
                float cur_z;
                z_axis_get_position(&cur_z);
                if (g_absolute_mode) {
                    target_z = cmd.z;
                } else {
                    target_z = cur_z + cmd.z;
                }
                float feed = cmd.has_f ? cmd.f : g_last_feed_mm_min;
                if (target_z < 0.0f || target_z > WORKSPACE_Z_MAX_MM) {
                    const char *err = "ALARM: Movimiento fuera del área de trabajo (Z)\r\n";
                    HAL_UART_Transmit(&huart2, (uint8_t*)err, strlen(err), 100);
                } else {
                    z_axis_move_to(target_z, feed);
                }
            }
        }
    }
    /* respondo ok al final de cualquier linea que haya llegado hasta
     * aca (haya hecho algo o no), porque laser grbl necesita este ok
     * para saber que puede mandar la siguiente linea de la cola */
    HAL_UART_Transmit(&huart2, (uint8_t*)"ok\r\n", 4, 100);
}

/* interrupcion de recepcion de uart: se dispara una vez por cada byte
 * que llega. ojo que esto corre en contexto de interrupcion, asi que
 * tiene que ser lo mas rapido y simple posible - por eso nunca
 * transmite nada directo, solo guarda bytes en el buffer circular o
 * prende banderas para que el main() se encargue despues */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rx_byte == '?') {
            /* '?' es un comando de "tiempo real" de grbl: no es parte
             * de una linea, pide el estado actual ya mismo. no lo meto
             * en el buffer de lineas, solo prendo la bandera */
            status_report_pending = 1;
        } else if (rx_byte == 0x18) {
            /* Ctrl-X: comando de soft-reset de Grbl. LaserGRBL lo manda
             * al conectar esperando ver de nuevo el mensaje de bienvenida. */
            welcome_pending = 1;
        } else {
            /* byte normal de gcode: lo guardo en el buffer circular.
             * si el buffer esta lleno (no deberia pasar nunca con 256
             * bytes de margen) simplemente se pierde ese byte en vez
             * de romper algo */
            uint16_t next_head = (rx_ring_head + 1) % RX_RING_SIZE;
            if (next_head != rx_ring_tail) {
                rx_ring[rx_ring_head] = rx_byte;
                rx_ring_head = next_head;
            }
        }
        HAL_UART_Receive_IT(huart, &rx_byte, 1); /* rearmo la recepcion para el proximo byte */
    }
}

/* interrupcion externa de los 3 finales de carrera. esto es lo que da
 * la parada de emergencia de verdad: si se activa un switch en medio
 * de un movimiento normal (no durante el homing a proposito), corta
 * todo al instante sin importar en que estaba ocupado el firmware en
 * ese momento */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (homing_is_active()) {
        /* si estamos homeando, tocar el switch es justo lo que se
         * espera que pase, asi que no hago nada aca: lo maneja el
         * propio homing.c con su propio polling */
        return;
    }

    if (GPIO_Pin == X_LIMIT_PIN || GPIO_Pin == Y_LIMIT_PIN || GPIO_Pin == Z_LIMIT_PIN) {
        stepper_emergency_stop();
        z_axis_emergency_stop();
        laser_disable();
        g_pending.valid = false; /* descarto cualquier movimiento que hubiera quedado en cola */

        /* los switches mecanicos rebotan (hacen varios contactos en
         * milisegundos por una sola pulsada fisica), asi que esta
         * interrupcion se puede disparar varias veces seguidas por un
         * solo choque real. por eso solo prendo la bandera del mensaje
         * la primera vez, para no mandar el aviso repetido un monton
         * de veces */
        if (!alarm_active) {
            alarm_message_pending = true;
        }
        alarm_active = true;
    }
}

/* interrupcion del timer: se llama cada vez que vence el periodo
 * configurado, tanto para tim2 (pasos de x/y) como para tim4 (pasos de
 * z). cada uno delega a su propio modulo */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        stepper_timer_isr();
    } else if (htim->Instance == TIM4) {
        z_axis_timer_isr();
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
