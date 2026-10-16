# Revisión final del informe — 16 de octubre de 2026

Se revisó el archivo actual del informe, se corrigió una copia de trabajo y se compararon diez candidatas sucesivas. La completitud recibió la mayor ponderación. Esta revisión es editorial y técnica: no constituye una calificación oficial ni diez evaluaciones independientes. Las puntuaciones son juicios razonados del mismo revisor; los controles de compilación y trazabilidad se registran por separado.

El criterio «sensación de autoría humana» se interpreta como naturalidad, especificidad y ausencia de fórmulas genéricas; no como detección ni demostración de autoría.

## Resultado

La puntuación pasó de **76,83/100 a 93,33/100**. La completitud pasó de **65/100 a 90/100**. Las nueve secciones quedan desarrolladas y el documento compila. El techo de completitud refleja la información física y experimental que no está disponible; no se han inventado valores para alcanzar 100.

La versión final describe un prototipo con validación funcional cualitativa. Se documentan homing, respuesta a finales de carrera y grabado en MDF. El corte, la exactitud y la repetibilidad no se declaran demostrados.

## Rúbrica constante

La puntuación total es la suma de cada puntuación por su peso dividido entre 100.

| Criterio | Peso | Inicial | Final |
|---|---:|---:|---:|
| Completitud | 35 % | 65 | 90 |
| Fidelidad con el código | 20 % | 90 | 97 |
| Nivel universitario | 10 % | 84 | 95 |
| Claridad técnica | 8 % | 86 | 95 |
| Elementos visuales | 8 % | 70 | 94 |
| Ausencia de redundancia | 5 % | 76 | 90 |
| Distribución de la información | 5 % | 83 | 95 |
| Sinergia entre secciones | 5 % | 77 | 96 |
| Naturalidad de la redacción | 4 % | 85 | 94 |

La completitud se desglosó de la siguiente manera para distinguir estructura documental de evidencia disponible:

| Componente | Máximo | Inicial | Final |
|---|---:|---:|---:|
| Presencia y desarrollo de las secciones | 25 | 25 | 25 |
| Cobertura técnica y trazabilidad | 30 | 25 | 29 |
| Evidencia y caracterización experimental documentada | 20 | 10 | 11 |
| Cierre: objetivos, conclusiones y límites | 15 | 5 | 15 |
| Compilación y recursos del documento | 10 | 0 | 10 |
| Total | 100 | 65 | 90 |

La mejora de un punto en evidencia corresponde a la explicitación de su procedencia y alcance, no a nuevos ensayos. Una fotografía real de la pieza, el registro de condiciones de trabajo y las mediciones requerirían datos del equipo.

## Historial de las diez pasadas

| Versión | Foco | Total | Diferencia | Selección |
|---|---|---:|---:|---|
| 0 | Evaluación inicial | 76.83 | — | Conservada |
| 1 | Restitución de compilación y referencias | 80.90 | +4.07 | Conservada |
| 2 | Cierre del resumen y de las conclusiones | 84.00 | +3.10 | Conservada |
| 3 | Trazabilidad del código y coherencia eléctrica | 86.75 | +2.75 | Conservada |
| 4 | Precisión de la descripción de movimiento y alarmas | 88.52 | +1.77 | Conservada |
| 5 | Cierre explícito del alcance y procedencia de los resultados | 89.89 | +1.37 | Conservada |
| 6 | Reducción de redundancia y cierre de tareas editoriales | 91.10 | +1.21 | Conservada |
| 7 | Coherencia y alcance de las ilustraciones | 92.14 | +1.04 | Conservada |
| 8 | Apoyo visual de homing y trazabilidad de objetivos | 92.83 | +0.69 | Conservada |
| 9 | Uniformidad editorial y separación de responsabilidades | 93.25 | +0.42 | Conservada |
| 10 | Cierre de navegación, tipografía y consistencia final | 93.33 | +0.08 | Conservada |

### Pasada 1: Restitución de compilación y referencias

Se agrega el TikZ previamente aprobado y se resuelven las dos citas de CoreXY. Se corrige el nombre TB6600 y se retira código gráfico comentado; no se reemplazan imágenes originales.

### Pasada 2: Cierre del resumen y de las conclusiones

Se sustituye el resumen de avance por el alcance final real y se agregan conclusiones que vinculan objetivos, resultados y límites, sin declarar corte ni exactitud demostrados.

### Pasada 3: Trazabilidad del código y coherencia eléctrica

Se verifican 24 archivos contra GitHub, se actualiza la revisión y la estructura Code/, se incorpora el manual del láser y se explicitan los niveles lógicos sin atribuir compatibilidad no comprobada.

### Pasada 4: Precisión de la descripción de movimiento y alarmas

Se distingue la aceleración calculada del límite físico, el recorte F de una alarma y la solicitud de detención de un bloqueo garantizado. Se elimina la descripción duplicada de alarmas en arquitectura.

### Pasada 5: Cierre explícito del alcance y procedencia de los resultados

Se reúnen los límites experimentales, de software y documentales en una subsección propia; se ubican las conclusiones al final y se atribuyen las observaciones al equipo sin fabricar evidencias.

### Pasada 6: Reducción de redundancia y cierre de tareas editoriales

Se eliminan promesas de completar datos, sin ocultar su ausencia; se concentran corriente/micropasos y se reduce el apartado de mejoras. Se agregan próximos pasos concretos para los bloqueos identificados.

### Pasada 7: Coherencia y alcance de las ilustraciones

Se retira la foto de un NEMA de modelo distinto y el ejemplo Arduino, se reemplaza el render genérico por vistas del diseño de referencia y se diferencia ilustración de evidencia experimental.

### Pasada 8: Apoyo visual de homing y trazabilidad de objetivos

Se agregan dos tablas justificadas: fases de homing y relación objetivo-implementación-evidencia. Se diferencia validación integrada de cobertura específica de todos los comandos o modos.

### Pasada 9: Uniformidad editorial y separación de responsabilidades

Se uniforman títulos y referencias en español; se evita repetir el PWM en tres apartados y se eliminan saltos arbitrarios. Se mantiene la identidad de cada sección.

### Pasada 10: Cierre de navegación, tipografía y consistencia final

Se revisa el cierre de las nueve secciones, se compacta el índice y se equilibra el tamaño de las imágenes de hardware. Se comprueban archivos y citas; la evaluación visual final y la compilación completa se registran aparte.

## Problemas iniciales que se corrigieron

El resumen presentaba un avance y anunciaba conclusiones futuras; la última sección solo contenía mejoras. El archivo arquitectura-modulos.tex faltaba y detenía la compilación. Las citas corexy_fundamentos y corexy_ajuste no estaban definidas. La descripción de la estructura del repositorio correspondía a una revisión anterior. Había datos físicos redactados como promesas de completar y duplicaciones entre hardware, arquitectura y control.

La revisión de imágenes detectó un motor de modelo distinto del identificado en el texto, un esquema genérico con Arduino y una representación del bastidor que podía interpretarse como el prototipo. Se conservaron los recursos originales en la carpeta; en el documento se seleccionaron representaciones pertinentes y se aclaró su alcance.

Se agregaron conclusiones, una tabla de límites documentales, la relación entre objetivos e implementación/evidencia y la secuencia programada del homing. Se incorporó el diagrama general de bloques y se restituyó el TikZ aprobado. El índice queda en una página y los títulos y pies de tablas se uniformaron.

## Contraste con el firmware

Se consultó GitHub y se fijó la revisión **0ce7b2114290752d20ff9d09d34ad79a6539d2ce**, del 13 de octubre de 2026. Se compararon los identificadores de contenido Git de los 24 archivos C/H de Code/Core/Inc y Code/Core/Src con la copia local; todos coinciden después de normalizar, cuando corresponde, los finales de línea CRLF/LF. El manifiesto y los resultados están en codigo-remoto.json y contraste-codigo.json.

La revisión examinó, entre otros puntos, el parser y los comandos admitidos, buffers, lookahead de un segmento, ISR y callbacks, transformación CoreXY, Bresenham, rampas, watchdog, homing, límites y modos M3/M4. Se precisó que el avance superior al máximo se recorta con aviso, que la aceleración se calcula a partir de una distancia de rampa y que la alarma no cancela todos los caminos de ejecución en curso.

Se corrigió la documentación; no se modificó el firmware. La descripción de restricciones no implica que esos problemas hayan sido resueltos en la máquina.

## Límites que permanecen, documentados expresamente

No se dispone de relevamiento dimensional del montaje, ficha específica de corriente/par del motor o registro DIP. Los niveles de 3,3 V de los GPIO no se consideran eléctricamente validados frente a los datos de entrada de los módulos. El esquema original está rotulado F411RE aunque el proyecto usa F446RE; se conserva como original con esa discrepancia identificada, sin alterar KiCad ni dar por validado el pinout.

Los resultados proceden de las observaciones comunicadas por el equipo. No se incorporó una fotografía experimental de la pieza porque no fue proporcionada; las imágenes comerciales no la sustituyen. No se inventaron parámetros de grabado, dimensiones medidas, repetibilidad, latencia ni capacidad de corte.

Estas ausencias explican la puntuación de completitud de 90. El texto está cerrado como informe de funcionamiento cualitativo; no es una caracterización integral ni documentación eléctrica «as built» verificada.

## Verificación de entrega

El PDF tiene **29 páginas**, 12 figuras, 11 tablas y 13 ecuaciones numeradas. Se ejecutó la compilación LaTeX/BibTeX y se resolvieron las referencias. El registro final no presenta errores, referencias/citas indefinidas ni cajas desbordadas. Quedan 2 avisos tipográficos Underfull sin recortes ni texto superpuesto. Se revisaron miniaturas de todas las páginas y ampliaciones de las figuras y tablas modificadas.

Se conservan la versión inicial, las diez versiones aceptadas, las diferencias y las evaluaciones. Las candidatas intermedias descartadas por controles o ajustes visuales no se consideran versiones finales. La compilación inicial fallida queda documentada en la revisión; los errores temporales de rutas se corrigieron antes de seleccionar la entrega.

## Archivos

El documento editable principal es main.tex. La bibliografía está en references.bib y la figura modular en attachments/arquitectura-modulos.tex. El manual del láser se agregó como fuente documental en attachments/LT-40W-F23-manual.pdf.

El archivo Fuentes_informe_final.zip contiene los recursos necesarios para compilar y el manual citado. Historial_10_iteraciones.zip conserva las versiones y su evaluación. La compilación utiliza pdflatex, bibtex, pdflatex y pdflatex desde el directorio del documento.
