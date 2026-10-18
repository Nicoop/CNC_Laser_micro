# CNC láser con STM32

Proyecto final de **Microcontroladores y Electrónica de Potencia**, de la carrera de Ingeniería Mecatrónica de la **Universidad Nacional de Cuyo**.

Se desarrolló el control de una CNC láser con movimiento **CoreXY** y ajuste independiente del eje Z, utilizando una placa **NUCLEO-F446RE**, drivers **TB6600** y motores paso a paso **NEMA 17**. El firmware está escrito en C y organizado en módulos.

## Funcionamiento

Las trayectorias se generan y envían desde **LaserGRBL** mediante G-code por comunicación UART, a través del puerto serie virtual del ST-LINK. El microcontrolador interpreta las instrucciones, coordina los pasos de los motores y controla el láser mediante PWM, con modos de potencia constante y dinámica.

Se incluyen referenciación de los tres ejes, finales de carrera, límites de software, entrada de parada de emergencia y supervisión mediante watchdog. La compatibilidad con GRBL comprende el conjunto de comandos implementado por el proyecto.

Se realizaron pruebas funcionales de referenciación, límites, parada de emergencia y grabado sobre MDF. La capacidad de corte no cuenta con validación documentada.

## Documentación

La descripción del hardware, las conexiones, los algoritmos y los resultados se encuentra en el **[informe del proyecto](Documentation/Entregables/Informe_Final_Microcontroladores_y_Electronica_de_Potencia.pdf)**.

Las [fuentes LaTeX](Documentation/Informe/) y el [proyecto eléctrico de KiCad](Documentation/DiagramaElectrico/DiagramaElectrico.kicad_pro) también están disponibles en el repositorio.

## Estructura

| Ruta | Contenido |
| --- | --- |
| `Core/` | Aplicación, parser, movimiento, homing, láser y supervisión. |
| `Drivers/` | Bibliotecas HAL y CMSIS de STM32. |
| `CNC_laser.ioc` | Configuración de periféricos y pines en STM32CubeMX. |
| `Documentation/` | Informe, esquemático, diagramas y referencias. |

## Desarrollo

El proyecto puede importarse en **STM32CubeIDE** y programarse mediante el ST-LINK de la NUCLEO. La configuración de pines y parámetros debe contrastarse con el montaje antes de utilizar la máquina.

## Licencia

El repositorio se distribuye bajo **GNU GPL v3**. Véase [LICENSE](LICENSE).
