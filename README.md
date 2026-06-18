Grúa con electroimán robotica Programable con LPC1769

Asignatura: Electrónica Digital III - Universidad Nacional de Córdoba

Integrantes:

•	Alanis, Gustavo alejandro
•	Marín Martinez, Mario Lautaro

Profesor: Marcos Javier Blasco 

________________________________________
1. Descripción General del Proyecto
El proyecto consiste en un brazo robótico de dos grados de libertad controlado mediante una placa NXP LPC1769. El sistema permite mover dos servomotores utilizando potenciómetros como interfaz de control manual y almacenar posiciones seleccionadas por el usuario.
Las posiciones almacenadas pueden reproducirse posteriormente de manera automática, permitiendo ejecutar secuencias de movimiento previamente grabadas. Además, el sistema incorpora comunicación UART con una interfaz desarrollada en Python para supervisión y selección de modos de operación.
________________________________________
 Alcances del Proyecto
El sistema SÍ es capaz de:
•	Controlar dos servomotores mediante potenciómetros.
•	Adquirir señales analógicas utilizando el ADC interno.
•	Utilizar DMA para transferir datos del ADC a memoria sin intervención constante de la CPU.
•	Almacenar posiciones de los servomotores mediante interrupciones externas.
•	Reproducir automáticamente la secuencia almacenada.
•	Accionar un electroimán mediante una salida digital.
•	Transmitir por UART las posiciones angulares actuales de los servomotores.
•	Permitir el cambio de modo Manual/Automático desde una interfaz Python.
________________________________________
 Posibles Etapas Futuras
•	Migración del prototipo a una PCB dedicada.
•	Incorporación de más grados de libertad.
•	Implementación de control mediante joystick.
•	Desarrollo de interfaz gráfica avanzada para monitoreo.
•	Incorporación de visión artificial mediante cámara externa.
•	Comunicación Bluetooth o WiFi.
________________________________________
 2. Arquitectura del Sistema
 Hardware e Interconexión
Componentes Principales
•	Microcontrolador NXP LPC1769
•	2 Servomotores
•	2 Potenciómetros
•	Electroimán
•	MOSFET de potencia
•	Conversor USB-UART
•	Fuente de alimentación de 5V
Diagrama de Bloques

<img width="676" height="1459" alt="image" src="https://github.com/user-attachments/assets/231ca05f-c93a-4cdb-be8c-1be7cdbaab32" />

 
Descripción del Circuito
Los dos potenciómetros generan señales analógicas que son digitalizadas mediante el ADC interno del LPC1769.
Las conversiones son transferidas mediante DMA hacia un buffer de memoria, reduciendo la carga de procesamiento.
Los servomotores son controlados mediante señales PWM generadas por timers.
El electroimán es accionado mediante un MOSFET de potencia y un diodo para protección inductiva.
________________________________________
Arquitectura de Software
Máquina de Estados
Modo Manual
•	Lectura ADC.
•	Actualización de PWM de ambos servos.
•	Envío de posiciones mediante UART.
•	Posibilidad de guardar posiciones.
Modo Automático
•	Lectura secuencial de posiciones almacenadas.
•	Actualización automática de ambos servomotores.
•	Reproducción continua de la secuencia.
________________________________________
 3. Especificaciones Eléctricas
Alimentación
•	Tensión de operación del LPC1769: 3.3V
•	Tensión de alimentación de servos: 5V
•	Tensión de alimentación del electroimán: 5V
•	Alimentación mediante fuente externa.
Consumo
Modo activo
•	LPC1769: ~100 mA
•	Servomotores: hasta 500 mA cada uno
•	Electroimán: variable según bobinado
________________________________________

IDE y SDK
•	MCUXpresso IDE
•	CMSIS v2
•	Drivers LPC17xx
Microcontrolador
•	NXP LPC1769
•	ARM Cortex-M3
Periféricos Utilizados
•	ADC
•	DMA
•	UART0
•	Timers 0 y 2
•	GPIO
•	NVIC
•	EINT1
•	EINT2
Estrategia de Concurrencia
Sistema mixto entre bare-metal y RTOS basado en:
•	Interrupciones externas.
•	Interrupciones de temporizadores.
•	Interrupciones UART.
•	Transferencias DMA.
________________________________________
 4. Proceso de Desarrollo
Etapa 1
Configuración del reloj y validación mediante LEDs.
Etapa 2
Implementación de generación PWM para servomotores.
Etapa 3
Implementación del ADC para lectura de potenciómetros.
Etapa 4
Integración del DMA para transferencia automática de muestras.
Etapa 5
Implementación de almacenamiento de posiciones mediante interrupciones externas.
Etapa 6
Implementación de reproducción automática.
Etapa 7
Comunicación UART con aplicación Python.
Etapa 8
Integración del electroimán.
________________________________________
5. Ensayos y Resultados
Pruebas Realizadas
Prueba ADC
Verificación de lectura correcta de ambos potenciómetros.
Prueba DMA
Verificación de transferencia continua al buffer de memoria.
Prueba PWM
Verificación de generación correcta de pulsos entre 500us y 2500us.
Prueba de Almacenamiento
Registro de posiciones mediante pulsador asociado a EINT2.
Prueba de Reproducción
Reproducción automática de secuencias almacenadas.
Prueba UART
Visualización de ángulos y modo de operación desde PC.
Prueba del Electroimán
Activación y desactivación mediante salida digital y MOSFET.
________________________________________


6. Estructura del Repositorio
├TP-Final
   ├TP.c
   ├RobotControl.py         
________________________________________
Conclusión
Se desarrolló un sistema embebido basado en LPC1769 capaz de controlar una grúa robótica de dos grados de libertad mediante adquisición analógica, DMA, temporizadores, interrupciones, comunicación UART y el control de un electroimán, mediante el micro. El proyecto permitió integrar múltiples periféricos avanzados del microcontrolador en una aplicación real de automatización y control.

