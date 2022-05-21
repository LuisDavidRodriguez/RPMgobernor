#pragma once


//DIGITALES
#define PIN_IMPRESORA 2			//pin no utilizado SERA PARA LA IMPRESORA
#define PIN_RPM 3				//(ESTE PIN CUANDO NO HAY PULSO DE PIN_RPM ESTA EN 1 GRACIAS A LA RESISTENCIA DE 10K, CUANDO ENTRA PULSO 12V AL TRANSISTOR ESTE CONDUCE Y MANDA A 0 EL PIN 2, SIGNIFICA QUE CUANDO HAY PULSO EN LA PIN_RPM EL PIN PASA DE 5V A 0 CONTAR LAS INTERRUPCIONES EN FALLING)
#define PIN_BLOQUEO 4			//(CUANDO NO HAY BLOQUEO GPS LA ENTRADA ESTA EN 1, CUANDO HAY BLOQUEO EL GPS MANDA TIERRA Y LA ENTRADA ES 0) CABLE AMARILLO GPS
#define PIN_CAJA 5				//pin donde esta el mosfet para abrir la caja de seguridad, inicializar como salida y en 0. al enviar 1 se activa el mosfet y abre la caja de seguridad
#define PIN_VENTILADOR 6		//pin donde estar conectado el mosfet para activar el ventilador cuando se active la alarma temperatura (INICIALIZAR COMO 0 CON 1 ENCENDEMOS EL VENTILADOR Y CON 0 LO APAGAMOS)
#define PIN_WDT 7				//Pin donde esta conectado el cable naranja del gps (CUANDO NO HAY PULSO GPS LA ENTRADA DE ARDUINO ESTA EN 1 CUANDO ENTRA UN PULSO CONDUCE EL TRANCISTOR QUE ENVIA LA ENTRADA A 0)
#define PIN_GLP 8				//pin donde se conectan las electrovalvulas para reconocer si el vehiculo esta ne modo gasolina o modo gas (CUANDO EL COCHE VA A GASOLINA ENTRA UN 0 EN EL CIRCUITO, LA ENTRADA DE ARDUINO ESTA EN 5V POR PULLUP, CUANDO CAMBIA A GLP ENTRA 1 EN EL CIRCUITO QUE HACE CONDUCIR EL TRANSISTOR ENVIANDO A LA ENTRADA DE ARDUINO UN 0)
#define PIN_SWITCH 9			//(CUANDO EL SWICH ESTA APAGADO EL PIN ESTA EN 1 CUANDO SE ENCIENDE EL SWICH, EL PIN CAMBIA A 0)
#define PIN_LUCES 10			//pin donde entra la señal digital luces (CUANDO LAS LUCES ESTAN APAGADAS EL PIN ESTA EN 1 CUANDO ESTAN ENCENDIDAS CAMBIA A 0)
#define PIN_BUZZER 11			//el pin 11 tiene el buzzer (EL BUZER SE ACTIVA CON 1 Y SE APAGA CON 0)
#define PIN_BOMBA 12			//el pin 12 donde esta la etapa de potencia para activar LA PIN_RPM DEL MOTOR (CON 1 ENCENDEMOS EL MOTOR Y CON 0 LO APAGAMOS)
#define PIN_AHORRO_ENERGIA 13	//es el pin utilizado para activar el mosfet que entrega la energia a todo el sistema (CON 1 ACTIVAMOS EL MOSFET Y CON 0 LO APAGAMOS) este esta integrado apartir del sistema 10.1, el mosfet corta la energia tanto del gps y tanto del sistema, el plan es que una vez que el swich del carro se mueva a on, activara el mosfet y se encendera tanto el gps como el gobernador, al encenderce el gobernador envia 1 por el pin 13 y activa el mosfet, en este momento aunque el swich del carro se apague el sistema seguira encendido, ya de ahi vasta programar el sistema para que despues de un determinado tiempo con el swich apagado, cambie el pin 13 a 0 y por lo tanto el capacitor antes del mosfet, hasta que se descarge y corte la eneria delk mosfet apagando todo el sistema en tehoria xD

//PINES ANALOGOS
#define PIN_ANG_NC_1 A0				//IS NOT USED ANYMORE.-entrada analogica por donde entra el voltaje del sensor de luz, el voltaje es vajo con poca luz e incrementa con mas luz. tiene una resistencia de 10k a tierra, la LDR esta a +5v y en su nodo entre 10k y ldr esta el pin arduino
#define PIN_ANG_BATTERY A1			//ENTRADA DEL VOLTAJE DE LA BATERIA GENERAL
#define PIN_ANG_TEMPERATURE A2		// pin donde estara conectada la entrada del sensor temperatura analogico del motor
#define PIN_ANG_NC_2 A3				//IS NOT USED ANYMORE.-pin analogico donde esta la lectura de la bateria del dvr
//A4 ES SDA DEL I2C
//A5 ES SCL DEL 12C
#define PIN_ANG_BOTONES A6			//PIN analogico DONDE ESTAN LOS BOTONES
#define PIN_ANG_NC_3 A7				//IS NOT USED ANYMORE.-En el pin analogo 6 esta el medidor de gasolina
