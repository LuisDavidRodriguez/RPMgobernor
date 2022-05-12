/*
 Name:		gobernadorRPM.ino
 Created:	30/11/2021 10:44:58
 Author:	david
 Prueba git 04-02-2022
*/




#include <LiquidCrystal_I2C.h>
#include <Wire.h> 
#include <EEPROM.h>
#include <avr/wdt.h> //libreria para el manejo de whatch dog timer recordar que hay que actualizar el bootloader del arduino nano
//LiquidCrystal_I2C             lcd(0x27, 2, 1, 0, 4, 5, 6, 7);
LiquidCrystal_I2C             lcd(0x27,16,2); //0x27 equals to 39


//#include <OneWire.h>                 //Se importan las libreríaspara leer el sensor de temperatura digital
//#include <DallasTemperature.h>       //libreria para leer el sensor de temperatura digital ds18b20
//#define Pin 2                        //Se declara el pin donde se conectará la DATA del sensor digital ds18b20
//OneWire ourWire(Pin);                //Se establece el pin declarado como bus para la comunicación OneWire
//DallasTemperature sensors(&ourWire); //Se llama a la librería DallasTemperature


//PINES ANALOGOS
byte sensorLuz = A0;//entrada analogica por donde entra el voltaje del sensor de luz, el voltaje es vajo con poca luz e incrementa con mas luz. tiene una resistencia de 10k a tierra, la LDR esta a +5v y en su nodo entre 10k y ldr esta el pin arduino
//byte lm2917=A0;//entrada analogica por donde entra el voltaje de las rpm del lm2917
byte pinbg = A1;//ENTRADA DEL VOLTAJE DE LA BATERIA GENERAL
byte pinT = A2;// pin donde estara conectada la entrada del sensor temperatura analogico del motor
//byte pinbateriadvr=3;//pin analogico donde esta la lectura de la bateria del dvr
//A4 ES SDA DEL I2C
//A5 ES SCL DEL 12C
int b = A6;//PIN analogico DONDE ESTAN LOS BOTONES
//int pingasolina=A7;//En el pin analogo 6 esta el medidor de gasolina


//DIGITALES
byte na0 = 2;               //pin no utilizado SERA PARA LA IMPRESORA
byte bobina = 3;            //(ESTE PIN CUANDO NO HAY PULSO DE BOBINA ESTA EN 1 GRACIAS A LA RESISTENCIA DE 10K, CUANDO ENTRA PULSO 12V AL TRANSISTOR ESTE CONDUCE Y MANDA A 0 EL PIN 2, SIGNIFICA QUE CUANDO HAY PULSO EN LA BOBINA EL PIN PASA DE 5V A 0 CONTAR LAS INTERRUPCIONES EN FALLING)
byte pingps = 4;              //(CUANDO NO HAY BLOQUEO GPS LA ENTRADA ESTA EN 1, CUANDO HAY BLOQUEO EL GPS MANDA TIERRA Y LA ENTRADA ES 0) CABLE AMARILLO GPS
byte pinCajaSeguridad = 5;               //pin donde esta el mosfet para abrir la caja de seguridad, inicializar como salida y en 0. al enviar 1 se activa el mosfet y abre la caja de seguridad
byte ventilador = 6;        //pin donde estar conectado el mosfet para activar el ventilador cuando se active la alarma temperatura (INICIALIZAR COMO 0 CON 1 ENCENDEMOS EL VENTILADOR Y CON 0 LO APAGAMOS)
byte pinWDT = 7;           //Pin donde esta conectado el cable naranja del gps (CUANDO NO HAY PULSO GPS LA ENTRADA DE ARDUINO ESTA EN 1 CUANDO ENTRA UN PULSO CONDUCE EL TRANCISTOR QUE ENVIA LA ENTRADA A 0)
byte pinGasGasolina = 8;    //pin donde se conectan las electrovalvulas para reconocer si el vehiculo esta ne modo gasolina o modo gas (CUANDO EL COCHE VA A GASOLINA ENTRA UN 0 EN EL CIRCUITO, LA ENTRADA DE ARDUINO ESTA EN 5V POR PULLUP, CUANDO CAMBIA A GLP ENTRA 1 EN EL CIRCUITO QUE HACE CONDUCIR EL TRANSISTOR ENVIANDO A LA ENTRADA DE ARDUINO UN 0)
byte pinswich = 9;            //(CUANDO EL SWICH ESTA APAGADO EL PIN ESTA EN 1 CUANDO SE ENCIENDE EL SWICH, EL PIN CAMBIA A 0)
byte pinluces = 10;           //pin donde entra la señal digital luces (CUANDO LAS LUCES ESTAN APAGADAS EL PIN ESTA EN 1 CUANDO ESTAN ENCENDIDAS CAMBIA A 0)
byte buzzer = 11;             //el pin 11 tiene el buzzer (EL BUZER SE ACTIVA CON 1 Y SE APAGA CON 0)
byte bomba = 12;              //el pin 12 donde esta la etapa de potencia para activar LA BOBINA DEL MOTOR (CON 1 ENCENDEMOS EL MOTOR Y CON 0 LO APAGAMOS)
byte ahorroEnergia = 13;      //es el pin utilizado para activar el mosfet que entrega la energia a todo el sistema (CON 1 ACTIVAMOS EL MOSFET Y CON 0 LO APAGAMOS) este esta integrado apartir del sistema 10.1, el mosfet corta la energia tanto del gps y tanto del sistema, el plan es que una vez que el swich del carro se mueva a on, activara el mosfet y se encendera tanto el gps como el gobernador, al encenderce el gobernador envia 1 por el pin 13 y activa el mosfet, en este momento aunque el swich del carro se apague el sistema seguira encendido, ya de ahi vasta programar el sistema para que despues de un determinado tiempo con el swich apagado, cambie el pin 13 a 0 y por lo tanto el capacitor antes del mosfet, hasta que se descarge y corte la eneria delk mosfet apagando todo el sistema en tehoria xD

bool cambioEstadoWDT = false;




//VARIABLES PARA EL CALCULO DE LAS RPM, Y CORTE POR RPM
float rpm;//almacena la lectura analogica * variable ajuste formula para saber las revoluciones reales  
float ajuste; //sirve para ajustar las rpm dependiendo del voltaje que de cada carro aumentaremos este valor con un push y lo guardaremos en la EEPROM
int corterpm; // definimos la variable corte rpm aqui nos mostrara el resultado de Corte*20, es decir de byte a informacion rpm
byte corte; // variable donde se almacena el valor de corte en tipo byte de 0 a 255 para almacenarlo en 1 byte de la EEPROM, se efectua una multiplicacion *20 para conocer el valor en RPM si este valor tiene 150*20=3000RPM
byte maxrpm = 0;//almacena un 1 si el si se activo el corte por rpm, y un 0 si no esta activado esto es para mostrar un mensaje en la pantalla de apagado por RPM
volatile int numInterrupt = 0;
unsigned long millisRpm = 0;

//Parareconocer las señales paracitas en las rpm
int rpmDelPasado = 0;
bool rpmParasita = false;
byte contadorRpmParasita = 0;




//VARIABLES PARA LA FUNCION TEMPERATURA

float ajusteT;// factor de divicion por el del valor analogico a la convercion grados
//float temperaturaD;//temperatura digital almacenara el valor de la temperatura en digital ya sea la obtenida directo del sensor digital o al convertir la lectura analoga temperaturaAnaloga/ajustetemperatura
float temperaturaD2;//almacena la conversion diginal del sensor analogo
bool corteT;// 1 si se activo el corte por temperatura, 0 si no esta activo el corte.
bool alarmaT;//1 si se activo la alerta por temperatura 0 si no esta activa
byte limiteT;// definira el nivel donde sonara la alerta por alta temperatura
byte limiteTC;//limite temperatura corte, define el nivel de apagado del motor
unsigned long TCALOR;//variable que almacena el tiempo en millis necesaria para evitar los rebotes de activado del ventilador, si alcanza una alara de temperatura el ventilador se activa durante 5 segudos y luego se apaga, esto porque si no el ventilador se prendia y se apagaba a cada ratito
unsigned long TCALOR2;//variable que almacena el tiempo en millis necesaria para evitar los rebotes al cortar la bomba por sobre temperatura
//unsigned long tiempolecturasensor;//almacena el tiempo en millis para leer el sensor de temperatura
bool activartemperatura;//0 cuando las alarmas estan activas 1 cuando estan desactivadas 


byte temperaturaEstimada = 0; //la temperatura estimada dependiendo del tiempo que el motor este encendido o apagado usada para activar las alarmas de temp, su valor maximo sera 95
unsigned long tiempoDeMotorApagadoTemperaturaEstimada = 0;
unsigned long tiempoDeMotorEncendidoTemperaturaEstimada = 0;
bool temperaturaEstimadaLlegoMaximo = false; //para saber si la temperaturaEstimada llego a su maximo valor, y entonces ahora asignarle el valor real de la temperatura y no seguir aumentando 10 grados por cada minuto encendido,
unsigned long tiempoMinimoParaActivarAlarmasTemperatura = 0;

unsigned long tiempopromedio;//almacena millis para los promedios de temperatura
byte posicionlectura = 0;// en que numero de lectura va 1, 2 0 3 o 4 utilizado en promediado de lecturas analogicas
float sumaTemperatura = 0;//almacena la temperatura que se tomoa cada segundo para promediarla despues
bool termineDePromediarLecturas = false; //se usa en el promedidiado de lecturas analogicas, pasara a true cuando pase por primera vez al calculo de promedios, ya las variables inician en 0 y algunas alarmas o codigos se disparan con esas lecturas cuando son realmente falsas, por ejemplo la lectura de la bateria, despues de reiniciarse la lectura sera 0 voltios por el tiempo que tarde en promediar las lecturas, en ese lapso se podria activar la alarma de bateria bajka o de alternador lo cual no esta bien



//LUCES
bool Aluces;//1 SI LA ALARMA SE ACTIVA 0 SI NO SE ACTIVA
byte luces;// DIGITAL 0 PARA ENCENDIDAS 1 PARA APAGADAS




//IGNICION MOTOR
bool swich;// DIGITAL 0 cuando el swich esta en posicion de encendido y 1 cuando esta en posicion de apagado harware TRANSISTOR 2N2222A EMISOR A TIERRA COLECTOR A 10K Y 5V Y EN BASE 10 K
unsigned long Tswich;//variable que almacenara el tiempo en millis para la funcion ALARMA SWICH ABIERTO
bool Aswich = 0;//varaible almacena 1 cuando se activo la alarma por swich o 0 cuando esta desactivada 
unsigned long retardoencendido;//almacena los millis para hacer el cambio a ignicion 
bool ignicion = 1;//variable 1 cuando el swich esta en posicion de apagado y el motor a menos de 100rpm, 0 cuando el swich se gira a encendido y las revoluciones son mas de 800 y ademas pasan 5 segundos apartir de que el motor esta en marcha se pone en 1

//GPS
byte gps;// DIGITAL 1 PARA DESBLOQUEADO 0 BLOQUEADO, NO NECESITA HARWARE SOLO CONFIGURAR EL INPUT_PULLUP



// ESTADO DE LA BATERIA GENERAL
float bgD;//contendra el valor digital en voltios
byte Abg;// Alarma bg 1 alarma activa mientras el motor esta encendido 0 alarma desactivada, 2 cuando se activa la alarma mientras el motor esta apagado
float ajusteBG;// Constante de divsion, para la conversion a Voltios
unsigned long Tbateriabaja;//almacena el tiempo en millis, se usa para añadir un retardo y que la alarma por drenado de bateria no se active al dar marcha al vehiculo. leer bien su funcion en el area de estado bateria general    
float bgDSuma = 0; //almacena la suma de los valores tomados cada periodo de tiempo para calcular el promedio
bool alarmasBateriaActivas;//almacena si se activan o no las alarmas depende de lo que seleccionen en el menu. se guardara en la EEPROM



// VARIABLES PARA APAGAR LA PANTALLA CUANDO LA IGNICION ES APAGADO y con el sensor de luz
long tiempoluces = 60000;// variable que almacena el tiempo en milisegundos para que se apage la luz de fondo de la pantalla una vez que se pone en 1 el swich lo pongo a 4 minutos
unsigned long tiempolucesfinal;// variable que almacena el valor millis().
byte estadopantalla;//varaible 1 pantalla apagada 0 variable encendidaunsigned long apagarDespuesDeAlarma; //la usaremos para almacenar millis y si el sensor de luz apaga la pantalla, despues se activa una alarma, vamos a permitir que la pantalla se quede encendida 30 segundos antes de volver a apagarse
unsigned long apagarDespuesDeAlarma; //la usaremos para almacenar millis y si el sensor de luz apaga la pantalla, despues se activa una alarma, vamos a permitir que la pantalla se quede encendida 30 segundos antes de volver a apagarse




//variables para ingresar al menu con 1 solo boton

unsigned long actualizarimagen;
byte conteomenu;//sera un contador que ira almacenando cada que el bucle pase porr el menu y el boton este precionado para que llegado a cierto numero donde el boton estuvo precionado entre  
bool enmenu = 0;// 1 si esta activo el menu 0 para salir 

//VARIABLES DE CONFIGURACION
unsigned long retardo;//variable que almacena el valor millis usado para evitar que lugar aumente muy rapidamente.


//Cronometro de clientes
byte Minutos;
byte Segundos;
unsigned long Tcronometro;//tiempo en millis  para los audios
unsigned long Tcronometro1;//tiempo en millis para el conteo de los minutos 

//Utilitarias
unsigned long pantalla1;// almacena millis() por cada refresco para llevar el refresco de la pantalla
unsigned long Trele; //variable usada en activacion del motor 
bool rele; //variable usada en activacion del motor

unsigned long alarmas1;//almacenara el tiempo en millis desde que se activo la alarma
bool A;
bool G = 1;//varaibles A es para la pantalla de alarmas y G es para la pantalla general

//alarma recordatorio de dar reportes al apagar y al encender el motor
bool R1 = 1;
bool R = 0;
unsigned long Tresetpantalla;//almacena millis para resetear la coneccion con la pantalla cada cierto tiempo

//VARIABLES PARA APAGADO SUAVE DEL MOTOR AL ENTRAR EL GOBERNADOR
byte vecesdecorte = 0;//aumenta una vez cada que se activa el gobernador
unsigned long Tvecesdecorte;//almacena en millis para reinciciar el contador vecesdecorte a 0

byte conteobateriabaja = 0;//variable que almacena y aumenta 1 cada vez que se activa la alarma por bateria baja para uqe la alarma deje de sonar y descargar la bateria hasta que el swich del carro se vuelva a poner en on 

String modo = "";//cambia a "gasolina" "gas lp" dependiendo del modo

int conteoTemporal;

//VARIABLES DE ALARMA COMUNICATE
bool alarmaComunicate = 0;
unsigned long tiempoAlarmaComunicate;

//VARIABLES DE ALARMA USO DE COMBUSTIBLE BIFUEL
unsigned long tiempoRetardoBifuel;
bool alarmaBifuelActiva; //indica si el usuario activo esta alarma desde el menu de configuracion o la desactivo debera guardarse en la EEPROM
byte contadorAlarmaBifuel = 0; //cuenta el numero de veces que se ha activado la alarma para asi activarla solo un numero de veces durante un periodo de tiempo, este aumentara cada vez que se entre a la pantalla de esta alarma
bool alarmaBifuel = 0;          //es la utilizada por la pantalla de alarmas
unsigned long tiempoBorrarBifuel; //guarda millis para borrar el contador de bifuel

//APAGAR LUZ POR SENSOR DE LUZ
int sumaSensorLuz = 0; //debido a que tomamos varias lecturas del sensor para eliminar ruidos aqui acumulamos las sumas para despues promediarlas
int promedioSensorLuz = 0; //esta es la lectura que se usara para las medidas de luz
bool luzDeFondoSensor = false; //este sera true cuando el sensor de luz detecte luz y quiera la luz encendida, false cuando el sensor de luz reconosca poca luz y quiera apagada la pantalla
int limiteInferiorSensorLuz;//se guardara en la EEProm y se modificara en el menu, por debajo de este valor se apaga la luz

//AHORRO DE ENERGIA
unsigned long tiempoAhorroEnergia;
byte contadorAhorroEnergia; //aumentara 1 a su valor cada que pase el tiempo programado para apagar el sistema por ejemplo cada 30 minutos aumentara 1 
byte enteroLimiteAhorro; //sera donde se guardara el entero por ejemplo 28 entonces 28 *30 munutos seran los minutos limites, no puede ser menor que 2 


bool bloqueoRapidoMotor;
bool puenteVentilador;
byte conteoPuenteVentilador = 0;
byte conteoLimpiezaRelay = 0;
byte conteoAbrirCajaFuerte = 0;

bool alarmaPuenteVentilador = true;
byte contadorAlarmaPuenteVentilador = 0;
unsigned long tiempoAnteriorAlarmaPuenteVentilador;
byte contadorAlarmaPuenteVentiladorEncedido = 0;
bool alarmaPuenteVentiladorEncendido = false;

//VERSION 10.6
byte conteoReiniciarTemperaturaEstimada = 0;

//VERSION 10.6---



const byte BOTON_4_SALIR = 20;
const byte BOTON_1_ACEPTAR = 30;
const byte BOTON_2_ABAJO = 40;
const byte BOTON_3_ARRIBA = 50;



const String VERSION_SOFTEARE = "10.8";
//version 10.1
// incluye la funcion de apretura de caja fuerte, se activa con el swich abierto sin encender el motor precionando el boton de salir, una ves ingresado el codig variable
//se debe de cerrar el swich para que el sistema abra la caja fuerte si la contraseña fue correcta
//Cambio del metodo de entrada al menu en lugar de teclear botones arriba y abajo cambiamos al ingreso por codigo escribiendo la clave 42341432 y esperando a que se agote el tiempo

//cambios en la version 10.2
//los botones de los sistemas cambiaron, los que incluyen los botones en el circuito cambiar la funcion botonera
//los sistemas que tienen un teclado externo color azul igual cambiar a su respectivo funcionamiento en la fucnioon botonera fue error de diseño en los circuitos mio
//Añadimos el puente del ventilador, dejando presionada la tecla de arriba por 10 segundos, hace que cada que el swich se active el ventilador se encienda y quede asi siempre
//se desactiva de igual manera presinonando durante 10 segundos el boton arriba y vuelve a su operacion normal.

//cambios aplicados en la version 10.3 :
// solucion al error de las Rpm paracitas que hacian que el sistema cortara el motor, por ruidos en la linea de revoluciones
// anidado a ese error de las RPM, reducimos el tiempo de muestreo de la pantalla que es el lugar donde el arduino calcula las revolcuiones actuales del motor
// bajamos el tiempo de muestreo de 250ms a 125ms 
// calculamos la temperatura ficticia para que el sistema active las alarmas de temnperatura dependiendo del tiempo que el motor lleva apagado y encendido
//las mostramos con el motor encendido ignicion 0 y apagado pero con el swich en off al presionar el boton salir como Ts (temperaturaEstimada) y como E (estado de alarmas temperatura 0 activas 1 inactivas)
//se añade la funcion limpieza de relay esta se activa precionando el boton abajo por 7 segundos
//se baja el tiempo del conteo del menu variable tiempoRegresivo para acceder de 20 a 10segundos
//se añade la version del software al presionar el boton salir con el motor apagado ignicion 0 y motor encendido ignicion 1, si se preciona con el motor apagado y el swich abierto entra a la funcion de caja fuerte
//se añade un contador para poder abrir la caja fuerte, se tiene que abrir el swich con el motor apagado y dejar presionada la tecla salir por 7 segundos antes se entraba con solo presionarla

//cambios aplicados en la version 10.4
//se mejora el acceso a la caja fuerte, sigue estando el metodo de abri el swich y presionar el boton salir 5 segundos, pero creo que seria un poco molesto porque
//en ese momento se activa la alarma de swich abierto y confunde al usuario
//ahora añadimos el acceso presionando el boton aceptar como si quicieramos acceder al menu, lo que ocurrira es que mostrara el numero de codigo variable a ingresar
//y tambien permitira el ingreso de la contraseña normal para el menu, se aumento el tiempo a 20 segudos para que de oportunidad de buscar el codigo variable
//entonces al agotarce el tiempo si se ingresa el codigo que corresponde al menu, entrra al menu, si se ingresa el codigo que concuerda con el codigo variable
//entonces abre la caja fuerte, y si no concuerda con nada pues vuelve al trabajo normal

//Cambios version 10.5
//se Añade la alarma del puente del ventilador, porque el puente del ventilador a veces los agentes lo activaban sin darce cuenta, y se quedaba asi por mucho tiempo
//hasta que el arnes se derretia, la alarma suena cuando se abre el swich, y cada 3 minutos que el motor va encendido.
//Se evita que las alarmas de swich avierto y puente ventilador se ensimen.
//se corrige error en temperatura estimada, si por ejemplo el motor estaba muy muy frio en la madrugada, y no alcanzaba a arrojar su temperatura correcta antes que se activara
//las alarmas de temperatura el sistema cortaba el motor y lo apagaba, al tratar de encenderlo nuevamente, las alarmas se activaban inmediatamente cuando ignicion pasaba a 0
//es decir reconocia el motor encendido, cuando eso ocurria volvia a apagar imediatamente el motor, a jovahni se lo hiso el vehiculo 12 y parese que si fue un problema
//porque el motor jamas podria estar encendido para calentarce debido a que el corte por temperatura lo apagaba, o se tendria que esperar con el motor apagado para que el
//sistema disminuyera la temperatura estimada y no activara las alarmas, pero deveria de esperar como 30 minutos.
//ahora lo que hacemos es darle un tiempo minimo despues de encender el motor de 1 minuto para activar las alarmas, aunque la temperatura estimada le indique que ya se
//pueden activar, no se activaran hasta transcurrido 1 minuto, asi si vuelve a repetirce el error, el motor podria funcionar 1 minuto para calentarce un poco.
//decidir si el minuto sera suficiente.


//Cambios version 10.6
// 
// para esta version notamos un posible problema importante, cuando nos envian un sistema para actualizar el software algunos sistemas se quedaron con la 
//  temperatura estimada demaciado alta, porque quitaron el sistema mientras el vehiculo aun estaba caliente, entonces como el sistema sigue sin energia
//  no se baja esa temperatura estimada debido a que deberia de reconocer el vehiculo apagado durante 10 minutos, el problema que afrontariamos
//  es que al enviar nuevamente el sistema el vehiculo claro ya esta frio pero el sistema sigue suponiendo una temperatura alta, lo cual podria probocar
//  que ese dia al conectarlo les active las alarmas de temperatura mucho antes de lo debido. para eso vamos a reiniciar la temperatura estimada
// presionando el boton salir por 10 segundos
// CANCELAMOS EL ACCESO A LA CAJA FUERTE ABRIENDO EL SWICH

//VERSION 10.7-- 10.8
//bajamos las revoluciones minimas del motor para que reconosca ignicion a 10 en la 10.8 no notamos que tambien habia que cambiar
//el renglon de abajo que cambia ignicion a 1 cuando las rpm son menores a 10, lo cambiamos en la 10.8





void setup() {
    wdt_disable();//desactivamos el watch dog

    pinMode(na0, OUTPUT);
    pinMode(pingps, INPUT_PULLUP);              //CONECCION CORTE GPS CON EL PULLUP SIMPLEMENTE EL PIN SIEMPRE MANDA 1 CUANOD EL GPS NO ESTA A TIERRA O ESTA DESBLOQUEADO  Y CUANDO EL GPS MANDA A TIERRA SE PONE A 0
    pinMode(pinCajaSeguridad, OUTPUT);
    pinMode(ventilador, OUTPUT);
    pinMode(pinWDT, OUTPUT);           //SIRENA 1 SIN PULSO 0 CON PULSO
    resetWDTexterno();
    pinMode(pinGasGasolina, INPUT_PULLUP);                 //PIN QUE RECIBE DEL TRANSISTOR 1 CUANDO ESTA EN MODO GASOLINA Y 0 CUANDO ESTA EN MODO GAS, CUANDO LA ELECTROVALVULA LE MANDA 12V A LA BASE DEL TRANSISTOR SE PONE EN 0 EL COLECTOR  
    pinMode(pinswich, INPUT_PULLUP);            // lectura de la posicion del swich
    pinMode(pinluces, INPUT_PULLUP);
    pinMode(buzzer, OUTPUT);
    pinMode(bomba, OUTPUT);
    pinMode(ahorroEnergia, OUTPUT);

    digitalWrite(ahorroEnergia, HIGH);        //activamos inmediatamente el mosfet que d ala energia al sistema
    digitalWrite(bomba, HIGH);               //activamos la bomba de gasolina
    digitalWrite(buzzer, HIGH);delay(20);digitalWrite(buzzer, LOW);
    digitalWrite(ventilador, HIGH);
    delay(50);
    digitalWrite(ventilador, LOW);
    digitalWrite(na0, LOW);
    digitalWrite(pinCajaSeguridad, LOW);




    // sensors.begin();                        //Se inician los sensores de temperatura digital ds18b20

    //Leemos desde la memoria EEPROM los valores de las varaibles 
    EEPROM.get(0, corterpm);
    EEPROM.get(2, ajuste);
    EEPROM.get(6, limiteT);
    EEPROM.get(7, limiteTC);
    EEPROM.get(8, ajusteT);
    EEPROM.get(16, ajusteBG);
    EEPROM.get(20, alarmaBifuelActiva);
    EEPROM.get(21, contadorAhorroEnergia);
    EEPROM.get(22, enteroLimiteAhorro);
    EEPROM.get(23, limiteInferiorSensorLuz);
    EEPROM.get(25, alarmasBateriaActivas);
    EEPROM.get(26, bloqueoRapidoMotor);
    EEPROM.get(27, puenteVentilador);
    EEPROM.get(28, temperaturaEstimada);






    lcd.init();
    //lcd.setBacklightPin(3, POSITIVE);
    lcd.setBacklight(HIGH);//iniciamos el LCD.
    attachInterrupt(digitalPinToInterrupt(bobina), funcionInterrupcion, FALLING);
    //wdt_enable(WDTO_4S);//activamos el watch dog timer


    resetWDTexterno();
    delay(500);

}

void loop() {

    

      //reseteamos el watch dog con cada loop
    resetWDTexterno();

    swich = digitalRead(pinswich);
    gps = digitalRead(pingps);


    //if (ignicion == 1 && millis() > 10800000) { asm("jmp 0x0000"); }//linea para saltar a la direccion de la memoria donde se encuentra la funcion para reiniciar arduino
    //reiniciamos arduino cada dia en la noche una ves que el vehiculo esta apagado y tarda 3 horas asi formula 3H * 60M *60S *1000=10800000millis



    //APAGADO POR AHORRO DE ENERGIA.
       //el Hardware del circuito 10.1 incorpora un mosfet en la entrada de energia del sistema, ese mosfet mientras tenga un 1 en su "gate" permite el paso de a un rele que se cierra y pasa
       //energia tanto al GPS como al arduino y el sistema completo. El GPS consume entre 120 a 190 ma y ardiono con la luz de fondo del lcd consume 48ma
       //con la luz de fondo apagada consume 25ma.
       //con esto queda claro que el mayor consumo presente es el gps, si hacemos calculos una bateria de coche tiene entre 60ah, tomemos la peor parte
       //que contenga 40ah, si lo dividimos 40ah / 0.180 = nos duraria 222 horas / 24hr serian algo asi como 9 dias, pero con esto entendemos que a los 9 dias la
       //bateria estara completamente descargada ni para darle marcha al vehiculo, tomando en concideracion para que el vehiculo de marcha nos duraria a ojo de buen
       //cubero como 5 dias.
       //es importante porque cuando el coche se va al taller o se queda de respoaldo detenido en la bodega no nos podemos permitir que se descarge la bateria
       //o desconectarle la bateria para que no se descarge. 
       //Se incorporo este sistema de apagado, cuando el swich del vehiculo se encienda, energisara el mosfet lo cual activara todo el sistema, en ese momento arduino
       //envia ahora un 1 al mosfet lo cual hace que aunque el swich se apage, la energia se mantenga en todo el sistema, esto hasta que arduino decida entrar en el
       //ahorro de energia, cuando entre en este modo mandara un 0 al mosfet lo cual hara que se apage en automatico todo el sistema incluido el gps
       //se quedara asi hasta que se vuelva abrir el swich del coche.
       //Abra que decidir el tiempo que deseamos yo propondria un dia completo o 24 horas, y aparte me gustaria que cuando se active el bloqueo por gps el sistema no se pueda
       //dormir. porque por ejempolo si roban el vehiculo quisas sea necesaro que el sistema no se apague.




       //mientras el motor este encendido o el gps este en modo de bloqueo de vehiculo el tiempo se actualizara evitando que entre al renglon para el ahorro energia
    if (ignicion == 0 || gps == 0) {
        tiempoAhorroEnergia = millis();

        //reiniciamos la variable a 0 porque si no, supongamos que anteriormente el coche estaba apagado y conto 26 numeros de 30 min,
        //entonces lo encienden, cuando lo vuelvan a apagar faltara solo 1 hora para que entre en modo de ahorro lo cual esta mal
        //le decimos que sea mayor a dos para ahorrar lo mas posible escrituras en la eprom asi por ejemplo para que se reinicie a 0 deberan
        //haber pasado por lo menos 1 hora desde que estuvo parado

        if (contadorAhorroEnergia > 2) {
            contadorAhorroEnergia = 0;
            EEPROM.put(21, contadorAhorroEnergia);
        }
    }

    //como el tiempo solo se dejara de actualizar hasta que el motor este apagado o el gps no este bloqueado entonces no vale la pena poner la condicion de ignicion == 0
        //cada de que pasen 30 minutos el sistema aumentara un 1 en algun posicion de la eeprom, sabemos que la memoria eemprom cada bloque tiene una vida de entre 100,000 y 1,000,000 ciclos de escritura y borrado
        //tomamos el numero mas bajo 100mil escrituras y borrados. suponiendo que el tiempo de escritura fueran cada 30 minutos entonces cada 30 minutos escribiremos una vez en la memora
        //EEPROM esto nos dara que 100,000 * 30minutos  = 3,000,000 de minutos / 60 = 50,000horas / 24 = 2083.3 dias / 365 = 5.7 años = 5 años 255 dias hasta que el bloque de la memoria
        //se dañe, pero esto siempre que el vehiculo este apagado, ahora sabemos que el vehiculo dura apagado serca del 60% del tiempo entonces el 40% de 5.7 años = 2.28 años el vehiculo
        //estaba encendido y por lo tanto no se escribia en la eemprom, ahora sumamos esos 2.28 años a los 5.7 años nos daria 7.98 años antes de que la vida del modulo de la memoria eemprom 
        // llege a su fin en tehoria es tiempo suficiente de vida para el sistema, muchos otros componentes se dañaran antes del modulo de la memoria.
        //guardaremos el valor en la eemprom para que arduino aunque se reinicie sepa un valor aproximado de cuanto tiempo lleva apagado el vehiculo, es aproximado porque pueden llegar a 
        //ocurrir reinicios antes de que se cumplan los 30 minutos del tiempo para contar, esto hara que el tiempo transcurrido se perda, pero para este propocito funcionara bien
    //1800000
    if (millis() > tiempoAhorroEnergia + 1800000) {
        //* 30 minutos es igual a 30 *60s *1000ms = 1,800,000
        contadorAhorroEnergia++; //aumentamos 1 al valor del contador

        //preguntamos si el contador llego al numero que deceamos para activar el corte de energia
            //recordar que para este ejemplo cada 1 guardado en el contador equivale a que pasaron 30 minutos
            //entonces para saber cuanto tiempo queremos que pase deberemos multiplicar el numero que queremos * 30
            //yo quiero que el sistema se mantenga activo 36 horas 36 * 60min = 2160 minutos / 30min = 72 del contador
            //entonces 72 numero de contador * 30min (es el tiempo durante el cual aumenta 1 al contador) nos da los 2160 minutos
            //el numero que eligamos no debera pasar los 256 que admite una variable tipo byte
            //cambiamos a 14 horas 14*60=840/30=28
        if (contadorAhorroEnergia >= enteroLimiteAhorro) {
            contadorAhorroEnergia = 0;
            EEPROM.put(21, contadorAhorroEnergia);
            digitalWrite(ahorroEnergia, LOW); //APAGAMOS EL MOSFET, EN ESTE MOMENTO EL SISTEMA SE APAGA COMPLETAMENTE GPS Y GOBERNADOR OBTENEMOS 0 CONSUMO DE ENERGIA

        }
        else {
            EEPROM.put(21, contadorAhorroEnergia); //guardamos el nuevo valor del contador en la eeprom
            tiempoAhorroEnergia = millis();
        }
    }



    //CALCULAR EL TIEMPO PARA QUE SE ACTIVEN LAS ALARMAS DE TEMPERATURA DEPENDIENDO DEL TIEMPO QUE EL VEHICULO DURO APAGADO
    //esta funcion esta descrita en el documento de Documentacion RPM

    //escribimos en la EEPROM la temperatura estimada para no tener que reinciar las variables
    //solo cuando el valor sea 256 es decir que no se a seteado la variable en la eemprom entraremos a esta linea
    //que reiniciara el valor a 0, sino en cada carro que cargemos el programa tendriamos que ir a reiniciar variables para que se escriba el valor correcto
    //pero entonces tendriamos que volver a configurar todos los ajuestes
    if (temperaturaEstimada > 200) {
        temperaturaEstimada = 0;
        EEPROM.put(28, temperaturaEstimada);
    }

    if (ignicion == 1) {
        //si el motor esta apagado mantenemos actualizado el tiempoDeMotorTemperaturaEstimada
        //porque antes al encender el coche inmediatamente se subia 10 grados la temperatura estimada
        tiempoDeMotorEncendidoTemperaturaEstimada = millis();
    }




    if (ignicion == 0 && millis() > tiempoDeMotorEncendidoTemperaturaEstimada + 60000) {
        //identificamos cada minuto que el motor pase encendido
        tiempoDeMotorEncendidoTemperaturaEstimada = millis();
        tiempoDeMotorApagadoTemperaturaEstimada = millis();
        //lo mantenemos actualizado mientras el motor este encendido para que al apagarlo espere los 10 minutos para restarle
        //porque si no, al apagarlo inmediatamente restaba 3 grados.



        if (!temperaturaEstimadaLlegoMaximo) {
            //si al temperatura estimada no ha llegado a su maximo entonces aumentamos 10 grados por cada minuto que el motor pase encendido
            temperaturaEstimada += 10;

            if (temperaturaEstimada >= 100) {
                temperaturaEstimada = 100;
                temperaturaEstimadaLlegoMaximo = true;
            }
            EEPROM.put(28, temperaturaEstimada);
        }
        else {
            //si la temperatura estimada ya alcanzo su maximo de 100, tomamos en cuenta la temperatura actual del motor


            //para ahorrar escrituras en la EEPROM y prolongar la vida del bloque, no escribiremos cada que la temperatura cambie
            //pondremos un condicional que solo escriba cuando la temperatura nueva sea menor a la guardada en la memoria en 5 grados
            //aparte que a este bloque solo se entra cada minuto que el motor esta encendido
            if (temperaturaD2 <= temperaturaEstimada - 7 || temperaturaD2 >= temperaturaEstimada + 7) {
                temperaturaEstimada = byte(temperaturaD2);//le metemos a un byte un float a ver si no hay algun error
                EEPROM.put(28, temperaturaEstimada);
            }

            /*supongamos que se enciende el motor, pasan 10 minutos y la temperatura estimada alcanza su maximo de 100 grados, supongamos que el motor llego a 85 grados reales
              el siguiente minuto que le motor este encendido entrara a este condicional, se verifica que 85 es menor que 93, entonces se toma el valor de la temperatura del motor
              y se guarda en la eemprom, al siguiente minuto la temperatura del motor sube solo 3 grados como no es mayor que la temperatura estimada +7 es decir 92
              no se actualiza el valor sigue valiendo 85, ahora si al siguiente minuto llega a 92grados entonces si actualiza el valor. supongamos que lo apagamos en 92
              la temperatura comenzara a bajar 3 grados cada 10 minutos gracias a lo renglones de abajo, pero apartir de 92grados no de 100 que es lo que se queria. Se pone el
              margen de temperatura menor porque por ejemplo cuando el motor va funcionando la temperatura esta entre 98, baja 1 grado o sube 1 o baja 3 por el ventilador etc
              de esta manera no se actualiza y no se escribe en la eeprom hasta que la temperatura baje por lo menos 7 grados.
              en el momento en que se entra los renglones inferiores donde se comienza a restar 3 grados cada 10 minutos, la variable que indica que llego al maximo
              cambia a false, para que cuando el motor vuelva a encenderce el primer minuto suba 10 grados nuevamente la variable y asi hasta que alcance su maximo y vuelva a tomar
              el valor real de la temperatura

            */

        }





    }



    if (ignicion == 1 && millis() > tiempoDeMotorApagadoTemperaturaEstimada + 600000) {
        //identificamos cada 10 minutos que el motor este apagado
        tiempoDeMotorApagadoTemperaturaEstimada = millis();


        temperaturaEstimada -= 5;
        temperaturaEstimadaLlegoMaximo = false; //para que cuando vuelva a encenderce comience a subir de 10 en 10 cada minuto
        if (temperaturaEstimada <= 0) {
            temperaturaEstimada = 0;
        }
        EEPROM.put(28, temperaturaEstimada);

    }


    if (ignicion == 0 && temperaturaEstimada >= 75) {

        if (millis() > tiempoMinimoParaActivarAlarmasTemperatura + 60000) {
            activartemperatura = 0; //activamos las alarmas de temperatura

            //le damos un tiempo minimo aunque las alarmas ya se puedan activar hacemos que esperen forzosamente 1 minuto despues que se enciende el motor
            //ocurria un error muy feo.
        }

    }
    else {
        activartemperatura = 1; //descativamos las alarmas de temperatura
        tiempoMinimoParaActivarAlarmasTemperatura = millis();

    }











    if (millis() > Tresetpantalla + 15000) {
        if (estadopantalla == 0) {
            lcd.init();
            lcd.setBacklight(HIGH);
            Tresetpantalla = millis();
        }
        if (estadopantalla == 1) {
            lcd.init();
            lcd.setBacklight(LOW);
            Tresetpantalla = millis();
        }
    }

    //en este punto, inicializamos la panalla cada cierto tiempo, esto porque en las pruebas de los carros la pantalla al tener un cable de 3 metros
       //recogia señales paracitas de interferencia y eso hacia que comenzara a mostrar simbolos extraños y se quedara asi por un buen tiempo
       //al reiniciar la pantalla si en alguna ocacion muestra los simbolos solo se quedara asi hasta que se haga el reinicio del lcd y comensara a funcionar
       //correctamente, usamos la variable "estadodepantalla" esta la utiliza el apartado de apagado de la pantalla despues de 1 minuto que el swich se cerro
       // entonces si han pasado los ms establecidos desde el ultimo reset del lcd, entra al if donde dependiendo de si la lus de fondo de la pantalla esta encendida
       // o apagada, se reinicia en el mismo estado. si no ponia esto, cuando despues de apagar el swich la funcion de apagado de la luz de fondo, apagaba la luz
       // despues al pasar el tiempo para el reset del lcd este se iniciaba pero encendia la luz de fondo y esta ya no se volvia a apagar    

    //si el swich se gira a la posicion de encendido y el motor se pone arriba de 800RPM y ademas transcurres 5 segundos desde que las rpm son mayores a 100 y el swich ya no es 1
       //activar alarmas se usa para las alarmas de temperatura ya que una vez que el motor esta caliente por ejemplo 95 grados como el sensor esta ubicado en la culata, una vez que el motor
       //se apaga y se deja de bombear el agua, la temperatura de la cabeza comienza a subir si estaba en 95° sube hasta 102° segun las pruebas sube de 5 a 10 grados de donde se apago
       //y ahi se mantiene hasta que poco a poco se comienza a enfriar.
       //si activamos asi el motor y digamos la alarma para apagar el motor esta en 100 grados, se disparara la alarma y no dejara encender el motor hasta que se enfrie aunque los 95 grados a los
       //que se apago el motor sea una temperatura normal. Al encender el motor tarda aproximadamente 1 minuto en lo que vuelve a su temperatura donde se apago ya que el refrijerante se
       //vuelve a bombear y comienza a enfriar el sensor, por eso se añade la funcion activartemperatura la cual dara ese tiempo necesario para que el refrijerante entre y nos entrege la medida
       //real y entonces activara las funciones de temperatura.  


    if (swich == 0 && rpm > 5 && millis() > retardoencendido + 10000)
    {
        ignicion = 0;             //A los carros 52,55 y 29 poner 6 minutos 
        //if(millis()>retardoencendido+360000)//Modifcamos el tempo de esta lnea para el carro que trae el sensor de temperatura en la ECM ponemos 6 mnutos de espera los demas 1 minuto
        // {
        //    activartemperatura=0;
         //}
    }

    if (swich == 1 || rpm < 5) {
        retardoencendido = millis();
        ignicion = 1;
        //activartemperatura=1;
    }
    //mientras alguna de las 2 sea verdadera se sigue actualizando el tiempo e ignicion vale 1
    //hasta que las 2 sean falsas se deja de actualizar el tiempo entonces podra validarse el if de arriba 





 //calculo de rpm solo si ya pasaro 250ms
     //segun mi hoja de excel "Calculo de inyeccion" la bobina a 1000rpm se activaria en 1 segundo 8.3 veces como hay 2 pulsos en el tiempo de 1 serian 16 veces por segundo
     // dividido entre 4 queda a 4 pulsos cada 250 ms entonces 1000 / 4 queda multiplicador por 250
     //se pone un transistor inversor emisor a gnd base a resistencia 10k a gnd, base resistencia 1k a señal, colector resistencia 10k a 5v y de colector sale la señal 
     //digital invertida a arduino. la entrada del arduino siempre va a estar en 5v alto, cuando haya un pulso en la bobina es decir hay un 1 en la base del transistor
     //inversor, la entrada de arduino bajara a 0 durante el tiempo que dure el pulso. por eso ponemos que reconosca la interrupcion cuando pasa de alto a bajo.
     //ponemos entre la base y la gnd del transistor un capacitor ceramico 103 o 0.01uf 10nanos esto limpia la señal de entrada y nos entrega el contador los 16 pulsos
     //por segundo que corresponden a las 1000rpm y los pulsos se mantienen estables, si no se pone este capacitor el contador se va a 80 o 90 pulosos y aparte es muy ocilante
     // le puce un capacitor del 102 es decir 0.001uf y si filtra la señal se baja a 25 pulsos pero es muy variable sube a 30 baja a 20 etc. el mejor es el 103.
     // a 1000 rpm marca 16 pulsos, a 2000 marca 34 pulsos y a 3000mil marca 54 pulsos
     //para obtener el factor de multiplicacion 1000rpm / 16pulsos = 62.5 y de ahi si se quiere en milisegundos, dividir 1000ms / el numero de milis que se desea muestrear
     // y luego multiplicar el resultado por el factor, 1000/250 = 4----- 62.5*4 = 250
/* if (millis () > millisRpm + 500){
   detachInterrupt(digitalPinToInterrupt(bobina));
   rpm = numInterrupt * 31.25;
   conteoTemporal = numInterrupt;
   numInterrupt = 0;
   millisRpm = millis();

   attachInterrupt(digitalPinToInterrupt(bobina), funcionInterrupcion, CHANGE);
 }
 */








 //CORTE POR REVOLUCIONES


  //if ( millis() > Tvecesdecorte + 10000 ){vecesdecorte=0;Tvecesdecorte=millis();}
  //reiniciamos cada 10 segundos la varaible veces de corte a 0, esto hace que si el chofer activo muchas veces el gobernador, entre el apagado violento



    if (rpm >= corterpm) {

        if (!rpmParasita) {
            maxrpm = 1;
            //vecesdecorte++;
            //digitalWrite(bomba,LOW);
            digitalWrite(buzzer, HIGH);
            //delay(200);
        }

    }
    else {
        maxrpm = 0;

        if (!rpmParasita) {
            if (rpm >= corterpm - 250) {
                digitalWrite(buzzer, HIGH);
            }
            else {
                digitalWrite(buzzer, LOW);
            }
        }


    }



    //if ( rpm > corterpm && vecesdecorte < 15 ){maxrpm=1;vecesdecorte++;digitalWrite(bomba,LOW);AUDIO_BEEP();delay(70);}else{maxrpm=0;}
    // cada vez que activa el gobernador veces de corte aumenta 1 vez, hace que durante 15 activaciones el apagado del motor sea suave, pero si el chofer no entiende que debe dejar de acelerar al llegar a las 15 veces entra el apagado violento del motor 

    //if ( rpm > corterpm && vecesdecorte >= 15 ){maxrpm=1;vecesdecorte++;digitalWrite(bomba,LOW);AUDIO_BEEP();delay(700);}else{maxrpm=0;}
    // DESCOMENTAR ESTA LINEA PARA ACTIVAR EL CORTE CON RETARDO MS JUNTO CON LA LINEA DEL ACTIVADO Y APAGADO DEL MOTOR
    //apagado violento del motor si las veces que el chofer activo el gobernador en 10 segundos supera las 15 veces se activa el apagado violento


    // avisa con beep cuando las revoluciones estan cerca del corte
    //if ( rpm > corterpm - 200 && rpm < maxrpm == 0 ){AUDIO_BEEP();} checar este renglon creo que estaba mal escrito borro el "rpm < maxrpm"

    //if ( rpm < 50 ){rpm=0;}
    //renglon para que al estar apagado el motor no actualice a cada rato el digito de rpm por interferencias




   //Promediado de las lecturas de los sensores y niveles de voltaje analogicos

    if (millis() > tiempopromedio + 1000) //leemos cada segundo las lecturas analogicas
    {
        posicionlectura++;

        //LUCES
        luces = digitalRead(pinluces);if (swich == 1 && luces == 0) { Aluces = 1;A = 1; }
        else { Aluces = 0; }
        //si el motor se apaga y las luces siguen encendidas




          //DETECTAR EN QUE COMBUSTIBLE ESTA TRABAJANDO
        if (!digitalRead(pinGasGasolina)) {
            modo = "GasLP";
        }
        else {
            modo = "Gasolina";
        }


        //***************PARA LOS DE FERNAN NO MOSTRAREMOS EL COMBUSTIBLE SINO SI EL MOTOR ESTA ACTIVADO O DESACTIVADO#############
        //if (bloqueoRapidoMotor){
        //  modo = "Activado";
        //}else{
        //  modo = "Desactivado";
        //}




        if (posicionlectura <= 4)
        {
            sumaTemperatura += (1023 - analogRead(pinT)) / ajusteT;
            bgDSuma += analogRead(pinbg) / ajusteBG;
            sumaSensorLuz += analogRead(sensorLuz);
            tiempopromedio = millis();
        }
        //calculamos los promedios y asignamos a las variables correspondientes
        else
        {
            posicionlectura--;
            temperaturaD2 = sumaTemperatura / posicionlectura;
            bgD = bgDSuma / posicionlectura;
            promedioSensorLuz = sumaSensorLuz / posicionlectura;

            tiempopromedio = millis();
            posicionlectura = 0;
            sumaTemperatura = 0;
            sumaSensorLuz = 0;
            bgDSuma = 0;

            termineDePromediarLecturas = true;
        }

    }//fin de Promediado



    //calculamos cuanto tiempo debera activarse el sensor de luz esto para que por ejemplo cuando
       //ha habido quejas de los agentes que en la noche cuando regresan de la ruta la luz de la pantalla les causa mucha incomodidad
       //entonces si el sensor de luz indica que hay poca luz en el exterior apagaremos la luz de fondo de la pantalla
       //esto mientras el vehiculo este encendido. la luz debera activarse siempre que entre una alarma al sistema o se precione un boton del sistema
       //pero de eso nos encargaremos en la parte de codigo que apaga la luz despues de determinado tiempo
       //el codigo solo se ejecutara hasta que se allan tomado las lecturas del sensor, porque por ejemplo al reiniciarse arduino, la lectura de luz sera 0
       //lo cual hara que de inmediato le indique a la pantalla que debe apagarse porque no hay luz, y si la pantalla estaba encendida
       //creara un detalle desagradable

    if (promedioSensorLuz < limiteInferiorSensorLuz && ignicion == 0 && termineDePromediarLecturas == true) {
        //if (promedioSensorLuz < 300 && termineDePromediarLecturas == true){
        luzDeFondoSensor = false; //apagamos la luz de fondo
    }
    else {
        luzDeFondoSensor = true; //encendemos la luz de fondo
    }





    //Alarma para comunicarce con Kaliope
       //esta alarma nace de la necesidad de que en ocaciones los agentes en ruta no responden sus telefonos celulares porque stan distraidos o no los escuchan
       //lo que haremos sera utilizar la entrada del pin sirena del gps, consultaremos continuamente si el pin no ha cambiado de 1 (estado sin pulso) a 0 (hay pulso)
       //es decir cuando al gps se le envie el mensaje de "disarm" este emitira por su sirena un pulso doble, el sistema lo captara y entonces debera detonar
       //una alarma que le indique al agente que se comunique a la empresa. Despues de un tiempo definido ya sean 30 segundos el mensaje debera desaparecer solo.
   // if (digitalRead(pinWDT) == 0) {
        //alarmaComunicate = 1;
        //tiempoAlarmaComunicate = millis();
      //  A = 1;
    //}
   // else {
        //if (millis() > tiempoAlarmaComunicate + 30000 && alarmaComunicate == 1) {
        //    alarmaComunicate = 0;

      //  }
    //}

    //si la alarma comunicate sige activa segimos poniendo 1 en el valor de A porque si no lo que ocurria era que a la primera ves
    //se activava la alarma, pero en el codigo de la pantalla, una ves que muestra una alarma cambia el valor de A = 0
    //con las demas alarmas no hay problema porque por ejemplo las luces siguen encendidas y por lo tanto su codigo sigue mandando A=1
    //pero como esta alarma solo recibe un pulso del gps el cual desaparece.
    //if (alarmaComunicate == 1 && A == 0) {
    //    A = 1;
    //}



    //Alarma de uso de combustible
       //la finalidad de esta alarma es para cuando el vehiculo es bifuel y utiliza gas lp y gasolina si el vehiculo viaja por mucho tiempo a gasolina nos saltara una alarma
       //donde le indique al chofer que el vehiculo va viajando a gasolina que por favor realice el cambio a gas lp, para realizar esto tomaremos en cuenta algunos estados del
       //vehiculo, por ejemplo despues de encender el vehiuculo deben de haber transcurrido minimo 60 segundos ya que el vehiculo inicia primero a gasolina, y luego a gas lp
       //pero creo que despues de encender el motor lo mejor seria que cuente 5 minutos que es el tiempo promedio que tarda en calentar un motor a mas de 50 grados para de esta
       //manera evitar las alarmas cuando el auto esta frio. Activaremos la alarma solo mientras el motor este encendido, la activaremos 3 veces continuas y despues la desacti
       //varemos durante 5 minutos para que al chofer si ya se le acabo el gas lp no se le haga tedioso el sonido de la alarma tambien crearemos un sonido mas breve que el Audio Alarma
       //esta alarma debera poder ser desactivada desde el menu de configuraciones



    if (ignicion == 0 && millis() > tiempoRetardoBifuel + 300000 && digitalRead(pinGasGasolina) == 1 && alarmaBifuelActiva == true && contadorAlarmaBifuel <= 3) {//si se enciende el motor  y el tiempo desde que se apago es mayor a 5 minutos y el pin gas gasolina sigue enviandole que va a gasolina, y esta activada la alarma desde el menu, y tambien la alarma se a activado un numero de veces menor al contador de alarmas
        alarmaBifuel = 1;
        A = 1;
        tiempoBorrarBifuel = millis(); //actualizamos aqui el tiempo, ya que en el momento en que deje de entrar a este lugar es porque se lelgo al limite de repeticiones, apartir de ahi corren los 5 minutos para borrar el contador
    }
    else {
        alarmaBifuel = 0;
    }

    if (ignicion == 1) {//si el motor esta apagado
        tiempoRetardoBifuel = millis();
        alarmaBifuel = 0;
        contadorAlarmaBifuel = 0;
    }

    //ahora una vez que la alarma se halla activado 2 veces por ejemplo, dejara de sonar debemos hacer que pasados 5 minutos el contador de la alarma se reinicie a 0
    // y pueda volver a activarse. Nota al programador, si se cambia aqui el numero 2 por otro numero este debera cambiarse tambien en el renglon superior donde se consulta si el 
    //contador es menor o igual al 2

    if (contadorAlarmaBifuel >= 3 && millis() > tiempoBorrarBifuel + 300000) {//si el contador de la alarma ya llego al numero definido de veces para activarse, esperara 5 minutos para reiniciarla
        contadorAlarmaBifuel = 0;
    }


    //FINAL DE USO DE COMBUSTIBLE BIFUEL  


    //PUENTE VENTILADOR PARA CONECTAR EL VENTILADOR EN CUNATO SE CIERRE EL RELEVADOR

    if (puenteVentilador == true) {
        //si se activo la funcion de puentear el ventilador cerramos el relevador del ventilador cuando el motor este encendido
        //si el puente del ventilador no esta activo el ventilador si en algun caso se quedara encendido cuando se desactiva 
        //lo apagaria la funcion de temperatura
        if (swich == 0) {
            digitalWrite(ventilador, HIGH);
        }
        else {
            digitalWrite(ventilador, LOW);
        }

    }



    //TEMPERATURA
    //sensors.requestTemperatures();temperaturaD=sensors.getTempCByIndex(0); Leemos la temperatura del sensor cada 5 segundos, requesTemperatures es para enviar un comando al sensor digital ds18b20 que prepare los datos de temperatura, despues a la variable temperaturaD, le asignamos el valor que nos devuelba el sensor en grados centigrados


    if (swich == 1) { temperaturaD2 = 0; }
    //Si el switch esta apagado hacemos que temperatura sea 0 para que no muestre -122 grados en la pantalla

    if (temperaturaD2 >= limiteT && temperaturaD2 < limiteTC && activartemperatura == 0) {
        alarmaT = 1;A = 1;
        digitalWrite(ventilador, HIGH);

        TCALOR = millis();
    }
    //si la temperatura es mayor al limitede alarma y menor al limite de corte, y transcurrio el tiempo para activartempertatura , se activa la alarma de temperatura, se enciende el ventilador y se guarda el tiempo de activado

    if (temperaturaD2 < limiteT || activartemperatura == 1) {
        alarmaT = 0;
        if (millis() > TCALOR + 180000) {
            if (puenteVentilador == false) {
                digitalWrite(ventilador, LOW);
            }
        }
    }
    //si la temperatura es menor al limite de alarma y han transcurrido 60s desde que se activo la alarma y durante el cual el ventilador esta encendido entonces se apaga la alarma
    //y el ventilador, primero apagamos la alarma, el ventilador se quedara encendido 60 segundos

    if (temperaturaD2 >= limiteTC && activartemperatura == 0) { corteT = 1;A = 1;TCALOR2 = millis(); }
    //si la temperatura es mayor o igaul al limite para cortar el motor y transcurrio el tiempo dado por activartemperatura despues de encender el motor para que el refrijerante entrara
    //y mostrara la temperatura real del motor activa el corte

    if (temperaturaD2<limiteTC && activartemperatura == 0 && millis()>TCALOR2 + 8000) { corteT = 0; }
    //cuando la temperatura decrese del limite de corte  espera 8 segundos para desactivar la alarma y volver a activar la bomba de gasolina,
    //este retardo se añade porque cuando alcansaba la temperatura de corte pero esta no era estable, volvia a bajar y despues volvia a subir el motor no se apagaba,
    //la boba se apagaba y se encendia, un rebote, con este retardo nos aseguramos que en cuanto la alarma detone se apage la bomba  

    if (activartemperatura == 1) { corteT = 0; }
    //para que cuando se apage el motor o el swich deje de sonar la alarma y active denuevo la bomba

     /*NOTA IMPORTANTE al usar el sensor analogico que esta en el motor el usar ignicion nos ayuda ya que cuando el swich esta apagado el sensor de
      * temperatura del carro no recibe voltaje como la temperatura la manejamos en inversa
      * es decir a menor voltaje mayor temperatura, al no haber voltaje la temperatura se va a 134 grados en el arduino, entonces cuando abres el swich o das marcha
      * esta alarma se detona y puede apagar el motor, al usar ignicion como trae retardo despues de reconocer 800 rpm en el motor hacemos que esta alarma no detone
      * hasta despues de cierto tiempo del motor andando*/






      //ESTADO DE LA BATERIA GENERAL  


    if (alarmasBateriaActivas) {//si las alarmas se activan en el menu

        if (bgD < 15.0 && bgD>12.7 || conteobateriabaja == 10) { Abg = 0; } //si el voltaje de la bateria esta entre 15 y 12.5v o el conteo de bateria baja a llegado a 10 apaga las alarmas
        else {


            if (bgD < 12.7 && ignicion == 0) {
                Abg = 1;
                A = 1;
            }//si el motor esta encendido,el voltaje decrese a menos de 12.5 se activa la alarma por falla de alternador, al usar la variable ignicion como tiene una condicional donde solo cambia a 0 despues de 5 segundos que el motor va a mas de 800rpm nos ayuda,a que no se active la alarma de falla de alternador cuando le damos marcha ya que el voltaje de la bateria se bajara y tarda un momento en meter carga el alternador 


            if (bgD <= 12.2 && swich == 1 && millis() > Tbateriabaja + 6000) { Abg = 2;A = 1; }//si swich es 1 es decir la llave esta en apagado, han pasado 6 segundos desde que se apago el swich y el voltaje decrese a menos de 11.8v se activa la alarma por drenado de bateria. Se agrega el retardo de 6 segundos despues del swich porque cuando se le da marcha al vehiculo, primero el swich arroja 0 es decir que esta en posicion de encendido, pero al dar marcha se corta la corriente del swich entonces entrega un 1, haciendo que se active la alarma de drenado de bateria mientras se le da marcha al carro. con el retardo, no se activara la alarma hasta pasado el tiempo
        }
        if (swich == 0) { Tbateriabaja = millis();conteobateriabaja = 0; }//mientras el swich este en posicion de encendido se actualiza el tiempo. y el contador de alarma de bateria baja se reinicia


    }
    else {
        //si las alarmas estan desactivadas en el menu
        if (Abg != 0) {
            Abg = 0;
        }


    }








    //ALARMA DE PUENTE DE VENTILADOR ACTIVADO, esta alarma nace debido a que sin saber los agentes activaban el puente del ventilador
    //y este se encendia a cada momento, duraba mucho tiempo asi encendido hasta que el arnes del circuito se derretia
    //la alarma sonara cada que se habra el swich del vehiculo, y cada 3 minutos que el motor este encendido

    if (puenteVentilador == true && swich == 0 && contadorAlarmaPuenteVentilador < 3) {
        alarmaPuenteVentilador = true;
        A = 1;

    }
    else {
        alarmaPuenteVentilador = false;
    }

    if (swich == 1) {
        contadorAlarmaPuenteVentilador = 0;
        alarmaPuenteVentilador = false;
    }


    //HACEMOS QUE SE ACTIVE CADA 3 MINUTOS QUE EL MOTOR ESTE ENCENDIDO
    if (ignicion == 0 && millis() > tiempoAnteriorAlarmaPuenteVentilador + 180000 && contadorAlarmaPuenteVentiladorEncedido <= 4 && puenteVentilador == true) {
        alarmaPuenteVentiladorEncendido = true;
        A = 1;
    }

    if (ignicion == 1) {
        //si el motor esta apagado mantenemos el tiempo actualizado
        tiempoAnteriorAlarmaPuenteVentilador = millis();
        alarmaPuenteVentiladorEncendido = false;
    }

    if (contadorAlarmaPuenteVentiladorEncedido > 4) {
        tiempoAnteriorAlarmaPuenteVentilador = millis();
        contadorAlarmaPuenteVentiladorEncedido = 0;
        alarmaPuenteVentiladorEncendido = false;
        //cuando haya sonado 4 veces la alarma actualizamos el tiempo y reiniciamos el contador, de esta manera volvera a esperar el tiempo deseado 3 minutos
        //para que vuelva a sonar 4 veces
    }








    //SWICH ABIERTO si el swich del carro esta en posicion de encendido y dura asi mas de 20 segundos y reconoce que las rpm son menores 100 entonces activa la alarma, en cuanto se apague el swich o se suban las revoluciones el la alarma se apaga
    if (swich == 0 && rpm < 10) {

        if (millis() > Tswich + 10000) {
            Aswich = 1;
            A = 1;
        }

    }
    else { Aswich = 0;Tswich = millis(); }

    //cronometro de clientes
    if (ignicion == 0) {
        Minutos = 0;Segundos = 0;
    }

    if (ignicion == 1 && millis() > Tcronometro1 + 1000) {
        Segundos++;Tcronometro1 = millis();//aumentamos la variable minuto cada 60mil mili segundos 
        if (Segundos > 59) { Segundos = 0;Minutos++; }
    }

    /*
    if (Minutos>=6&&millis()>Tcronometro+150){digitalWrite(buzzer,HIGH),delay(20);digitalWrite(buzzer,LOW);Tcronometro=millis();}else{//fucnones para los audios dwe alerta
    if (Minutos>=5&&millis()>Tcronometro+1000){digitalWrite(buzzer,HIGH),delay(20);digitalWrite(buzzer,LOW);Tcronometro=millis();}else{
    if (Minutos>=4&&millis()>Tcronometro+2000) {digitalWrite(buzzer,HIGH),delay(20);digitalWrite(buzzer,LOW);Tcronometro=millis();}else{
    if (Minutos>=3&&millis()>Tcronometro+5000) {digitalWrite(buzzer,HIGH),delay(20);digitalWrite(buzzer,LOW);Tcronometro=millis();}else{
      if (Minutos>=2&&millis()>Tcronometro+10000) {digitalWrite(buzzer,HIGH),delay(20);digitalWrite(buzzer,LOW);Tcronometro=millis();}else{
        if (Minutos>=1&&millis()>Tcronometro+10000) {digitalWrite(buzzer,HIGH),delay(20);digitalWrite(buzzer,LOW);Tcronometro=millis();}
      }
    }
    }
    }
    }
    */





    //ACTIVACION Y CANCELACION DEL MOTOR // si el swich esta en posicion de encendido el gps esta desbloqueado y no esta activo el corte por rpm ni el dorte por temperatura entonces encendemos la bomba y apagamos el led
    if (rele == 0 && gps == 1 && corteT == 0 && maxrpm == 0 && bloqueoRapidoMotor == true) {
        digitalWrite(bomba, HIGH);
        //digitalWrite(buzzer,LOW);
    }
    else {
        digitalWrite(bomba, LOW);
    }//LINEA PARA CORTE POR MS


    if (swich == 0) { rele = 0;Trele = millis(); }                            //mientras el swich este en posicion de encendido rele cambia a 0 es decir que se cumplira el if de activacion y cancelacion del motor
    if (swich == 1 && millis() > Trele + 15000) { rele = 1; }                     // si swich de gira a posicion de apagado, se tendra que esperar 15 segundos para que rele cambie a 1 y haga que ya no se cumpla la condicion de activacion del motor, lo cual hara que el relevador que activa la bobina se apage

    //unsigned long Trele; variable usada en activacion del motor 
    //bool rele; variable usada en activacion del motor
  /*se agrega estas lineas porque el rle usado en el corte de la bobina siempre se queda energisado aunque el motor esta apagado. estamos usando los contacots NA 30 y 87
   * es decir el motor solo encendera hasta que se energise el rele y esto es enviando un estado HIGH al pin bomba, el problema venia que al apagar el motor el rele se
   * quedaba encendido siempre.
   * mientras el swich este en posicion de encendido la variable rele cambia a 0 y actualizamos el tiempo en Trele. cuando la variable rele cambia a 0
   * hacemos que el if de activacion de la bomba se valide y por lo tanto el rele se energise y cierre sus contactos permitiendo que el motor encienda
   * al girar la llave a la posicion de apagado swich==1, el tiempo desde la ultima vez que swich valia 0 se queda almacenado en Trele, al sumarle los 15mil ms estamos esperando
   * 15 segundos, esto permite que el rele se mantenga energisado durante 15 segundos depues que el swich se puso en la posicion de apagado, despues del tiempo de espera rele cambia a 1
   * provocando que ya no se valide el if de activacion del motor, llendo a else donde se desenergisa el relay.
   * una vez que swich cambia a 0 es decir se quira la llave a on, rele cambia a 0 y se energisa el relay, recordemos que cuando se le da marcha al motor
   * swich cambia por un breve momento a 1, pero como tiene el retardo antes de que rele cambie a 1 da oportunidad de que el sistema arranque sin problemas,
   * si no pusieramos el retardo en cuando dieramos marcha el relay se desenergisaria y el motor no arrancaria
   */

   //alarma recordatorio de dar reportes
   /*
   if (ignicion==0&&R1==1){R=1;}
   if (ignicion==1&&R1==0){R=1;R1=1;}
   if (R1==0){R=0;}
   if (BOTONES()==20){R1=0;}*/





   //PANTALLA
    if (millis() > pantalla1 + 125) {//frecuencia de muestreo
     //PANTALLA GENERAL____________________________________________________________________

        detachInterrupt(digitalPinToInterrupt(bobina));
        //rpm = numInterrupt * 125; //cuando se cuentan 8 pulsos cada 250 milis, es decir cuando se toma la señal de ambos cables de la bobina con 1 diodo
        rpm = numInterrupt * 8; //cuando las tomamos del sensor de cigueñal el sensor de sigueñal entrega 60 pulsos por revolucion, cuando va a 1000rpm por segundo entrega 60 pulsos como contamos cada 250 ms tendriamos 15 pulsos por eso multiplicamos *4
        conteoTemporal = numInterrupt;
        numInterrupt = 0;

        //tenemos el problema de los ruidos en la linea, cuando vamos manejando a velocidad constante y marca 2200Rpm depronto en varias ocaciones
        //el buzzer suena y a veces apaga el motor, esto porque es como si entraran señales paracitas y deprotno el sistema marca 3200rpm
        //lo que haremos sera guardar las rpm en otra variable, esta se usara para comparar las nuevas revoluciones. se supone que calculamos las revolcuciones
        //cada 250ms, entonces si las revoluciones nuevas son mas altas por 800rpm que las revoluciones de hace 250ms entonces ignorara el corte
        //aun no se que podria ocurrir pero espero que si se hacelera intencionalmente el motor en 250ms este no alcance mas de 800rpm
        //contamos, si son dos veces seguidas las que se entraron a la señal paracita, entonces la asumimos como verdadero y actualizamos las rpm del pasado
        //usamos la variable rpmParacita en  la linea donde se activa la variable maxRpm en CORTE RPM      
        if (rpm > rpmDelPasado + 800) {
            rpmParasita = true;
            contadorRpmParasita++;

            if (contadorRpmParasita >= 2) {
                rpmDelPasado = rpm;
                contadorRpmParasita = 0;
                rpmParasita = false;
            }


        }
        else {
            rpmParasita = false;
            rpmDelPasado = rpm;
            contadorRpmParasita = 0;
        }




        //millisRpm = millis();




        if (G == 1 && BOTONES() == 0) {// cuando G sea igual a 1 va a entrar a la muestra de pantalla general
            lcd.clear();
            lcd.home();
            lcd.setCursor(0, 0);
            lcd.print(String(rpm, 0));
            lcd.setCursor(5, 0);
            lcd.print(String(bgD, 1) + "v");
            lcd.setCursor(11, 0);
            lcd.print(String(temperaturaD2, 0) + "C");//+ String(contadorAhorroEnergia));
            lcd.setCursor(0, 1);
            //lcd.print(" niL:" + String(promedioSensorLuz)+ " Luz:" + (luzDeFondoSensor ? "on" : "off"));//String (conteoTemporal));

            lcd.print(String(Minutos) + ":" + String(Segundos) + " " + modo + String(puenteVentilador == true ? "  V" : ""));//String (conteoTemporal));

            if (gps == 0 && swich == 0) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("MOTOR APAGADO"));
                lcd.setCursor(0, 1);
                lcd.print(F("LLAME A KALIOPE"));
                //AUDIO_GRAVE();
            }//si el gps se bloquea y el motor esta encendido, en cuanto se apage el motor la alarma dejara de sonar

        }//_______________________________________________________________________________


           //PANTALLA DE ALARMAS_________________________________________________________
        if (A == 1 && millis() > alarmas1 + 6000) {//si se activo alguna alarma entra a la pantalla de alarmas se queda 2 segudnos en el mensaje, despues va a pantalla general hasta que millis sea mayor a 5000-2000=3000
            G = 0;
            alarmas1 = millis();

            if (alarmaT == 1 && corteT == 0) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("Temperatura Alta"));
                lcd.setCursor(0, 1);
                lcd.print(String(temperaturaD2) + "C");
                lcd.setCursor(5, 1);
                lcd.print(F("VENTILANDO"));
                AUDIO_ALARMA();
            }

            if (corteT == 1) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("MOTOR FUERA DE"));
                lcd.setCursor(0, 1);
                lcd.print("TEMPERATURA" + String(temperaturaD2) + "C");
                AUDIO_GRAVE();
            }

            if (Aluces == 1) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("LUCES ENCENDIDAS"));
                lcd.setCursor(0, 1);
                lcd.print(F("APAGALAS!!!"));
                AUDIO_ALARMA();
            }

            if (alarmaComunicate == 1) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("LLAMA A KALIOPE"));
                lcd.setCursor(0, 1);
                lcd.print(F("POR FAVOR"));
                AUDIO_ALARMA();
            }


            if (alarmaBifuel == 1) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("CAMBIA A GasLP"));
                lcd.setCursor(0, 1);
                lcd.print(F("usas gasolina"));
                contadorAlarmaBifuel++; //incrementamos el contador de alarma
                AUDIO_LIGERO();
            }



            if (Abg == 1) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("FALLA DE"));
                lcd.setCursor(0, 1);
                lcd.print("ALTERNADOR" + String(bgD, 1) + "v");
                AUDIO_GRAVE();
            }



            if (Abg == 2 && alarmaPuenteVentilador == false && Aswich == false) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("BATERIA BAJA"));
                lcd.setCursor(0, 1);
                lcd.print(F("ENCIENDA MOTOR"));
                AUDIO_ALARMA();
                conteobateriabaja++;
            }//conteo bateria baja aumentara una vez cada que se active la alarma por drrenado de bateria, para que al llegar a un numer determinado de avisos se apage la alarma




            if (Aswich == 1 && alarmaPuenteVentilador == false) {
                //para evitar que se encimen las alarmas de swich abierto y del puente de ventilador de esta manera le damos prioridad a la del puente
                //antes que a la del swich
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("EL SWICH ESTA"));
                lcd.setCursor(0, 1);
                lcd.print(F("ABIERTO, APAGALO"));
                AUDIO_ALARMA();
            }


            if (alarmaPuenteVentilador == true) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("DESACTIVA PUENTE"));
                lcd.setCursor(0, 1);
                lcd.print(F("DEL VENTILADOR!!"));
                AUDIO_MEDIO();

                //for(int iterator = 0 ; iterator < 10; iterator++ ){
                //  lcd.scrollDisplayLeft();
                //  delay(400);

                //}

                contadorAlarmaPuenteVentilador++;

            }


            if (alarmaPuenteVentiladorEncendido == true) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(F("PUENTE VENTILADOR"));
                lcd.setCursor(0, 1);
                lcd.print(F("LLAMA A KALIOPE"));
                AUDIO_MEDIO();

                contadorAlarmaPuenteVentiladorEncedido++;

            }




        }
        else {
            A = 0;
        }


        if (millis() > alarmas1 + 2000) {
            G = 1;
        }//tiempo de retardon en donde se queda mostrando el mensaje de error y despues vuelve a la pantalla general




       //_____________________________________________________________
       //pantalla de Configuraciones__________________________________si precionamos el boton arriba nos muestra esta pantalla
        if (BOTONES() == BOTON_3_ARRIBA) {
            lcd.clear();
            lcd.setCursor(0, 0);lcd.print("C:" + String(corterpm) + " " + String(corterpm / 30) + "km/h");
            lcd.setCursor(0, 1);lcd.print("AT:" + String(limiteT) + "c TC:" + String(limiteTC) + "c");
        }

        if (BOTONES() == BOTON_2_ABAJO) {
            lcd.clear();
            lcd.setCursor(0, 0);lcd.print("M:" + String(digitalRead(bomba)) + " I:" + String(ignicion) + " L:" + String(luces) + " SW:" + String(swich));
            lcd.setCursor(0, 1);lcd.print("V:" + String(digitalRead(ventilador)) + " GLP:" + String(digitalRead(pinGasGasolina)) + " GPS:" + String(gps));

        }




        if (BOTONES() == BOTON_4_SALIR) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Luz:" + String(promedioSensorLuz) + " Lim:" + String(limiteInferiorSensorLuz));
            lcd.setCursor(0, 1);
            lcd.print("Ts:" + String(temperaturaEstimada) + "c " + "E:" + String(activartemperatura) + " " + VERSION_SOFTEARE);
            //mostramos la temperatura estimada que es la que el sistema calcula dependiendo del tiempo que le motor esta apagado o encendido
            //y tambien mostramos el valor de las alarmas de temperatura si es un 0 significa que el sistema ya las activo, si es un 1 significa que estan desactivadas



        }

        pantalla1 = millis();


        attachInterrupt(digitalPinToInterrupt(bobina), funcionInterrupcion, FALLING);
    }







    //ACCEDER AL MENU CONFIGURAR AL PRECIONAR 1 BOTON      
    if (BOTONES() == BOTON_1_ACEPTAR) {
        if (millis() > retardo + 1000) {
            retardo = millis();
            conteomenu++;
        }
        //si presionamos el boton entrar aumentamos la variable conteo menu, aumentamos 1 por cada segundo que pase 
    }
    else {
        conteomenu = 0;
    } //si soltamos el boton entrar o no esta precionado conteo se reinicia a 0 


    if (conteomenu >= 5) { //cuando el conteo del menu llege a esta cifra ingresamos al menu
        AUDIO_EXITO();
        if (ingresaAlMenu()) {
            //si la contraseña es correcta, ingresamos al menu
            delay(500);
            menuPrincipal();
        }
    }






    //ACCEDER A ACTIVAR PUENTE DE VENTILADOR     
    if (BOTONES() == BOTON_3_ARRIBA) {
        if (millis() > retardo + 1000) {
            retardo = millis();
            conteoPuenteVentilador++;
        }
        //si presionamos el boton entrar aumentamos la variable conteo menu, aumentamos 1 por cada segundo que pase 
    }
    else {
        conteoPuenteVentilador = 0;
    } //si soltamos el boton entrar o no esta precionado conteo se reinicia a 0 


    if (conteoPuenteVentilador >= 7) { //cuando el conteo del menu llege a esta cifra ingresamos al menu

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Puente ventilado");
        lcd.setCursor(0, 1);
        if (puenteVentilador) {
            //si el puente del ventilador esta activo entonces lo desactivamos
            puenteVentilador = false;
            EEPROM.put(27, puenteVentilador);//guarda un bool en la posicion 27 a 27 siguiente posicion 28   
            lcd.print("Desactivado");
        }
        else {
            puenteVentilador = true;
            EEPROM.put(27, puenteVentilador);//guarda un bool en la posicion 27 a 27 siguiente posicion 28
            lcd.print("Activado");
        }

        AUDIO_EXITO();
        delay(500);

    }





    //ACCEDER A ACTIVAR LA LIMPIEZA DE Relays m se activara presionando el boton abajo y con el swich apagado
    if (BOTONES() == BOTON_2_ABAJO && swich == 1) {
        if (millis() > retardo + 1000) {
            retardo = millis();
            conteoLimpiezaRelay++;
        }
        //si presionamos el boton entrar aumentamos la variable conteo menu, aumentamos 1 por cada segundo que pase 
    }
    else {
        conteoLimpiezaRelay = 0;
    } //si soltamos el boton entrar o no esta precionado conteo se reinicia a 0 


    if (conteoLimpiezaRelay >= 7) { //cuando el conteo del menu llege a esta cifra ingresamos al menu            


        funcionLimpiezaDeRelay();

    }






    //EN VERSION 10.6 ACCEDER AL MENU CONFIGURAR AL PRECIONAR 1 BOTON      
    if (BOTONES() == BOTON_4_SALIR) {
        if (millis() > retardo + 1000) {
            retardo = millis();
            conteoReiniciarTemperaturaEstimada++;
        }
        //si presionamos el boton entrar aumentamos la variable conteo menu, aumentamos 1 por cada segundo que pase 
    }
    else {
        conteoReiniciarTemperaturaEstimada = 0;
    } //si soltamos el boton entrar o no esta precionado conteo se reinicia a 0 


    if (conteoReiniciarTemperaturaEstimada >= 10) { //cuando el conteo del menu llege a esta cifra ingresamos al menu
        temperaturaEstimada = 0;
        EEPROM.put(28, temperaturaEstimada);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp estimada");
        lcd.setCursor(0, 1);
        lcd.print("Reiniciada a " + String(temperaturaEstimada));
        AUDIO_EXITO();
        delay(1000);

    }





    /*EN VERSION 10.6 ANULAMOS LA APERTURA DE LA CAJA POR ESTE MEDIO
    if(BOTONES() == BOTON_4_SALIR && swich == 0 && ignicion == 1){
        //para entrar al ingreso del codigo para liberar la caja hay que tener abierto el swich del coche y presionar el boton de salir, sin que el motor este encendido
        if (millis() > retardo + 1000){
              retardo=millis();
              conteoAbrirCajaFuerte ++;
            }


                //abreCajaFuerte();
                //bloqueoDeMotorRapidoFernan();
    }else{
          conteoAbrirCajaFuerte = 0;
    }

    if (conteoAbrirCajaFuerte>=7){ //cuando el conteo del menu llege a esta cifra ingresamos al menu
       abreCajaFuerte();

    }
    */













    //APAGAR LA LUZ DE FONDO DEL DISPLAY CUANDO SE RECONOCE EL APAGADO DEL MOTOR O POR EL SENSOR DE LUZ




      //si el coche esta encendido, y el sensor de luz le indica que debe apagarse la pantalla, este se apagara,
      //se encendera al activarse una alarma o al presionar un boton y se quedara activa 30 segundos      
      //si el motor esta encendido hay dos opciones con la luz de fondo, que el sensor indique que hay luz y la pantalla se mantenga encendida
      //o que el sensor le indique que debe apagarce

    if (swich == 0) {//MIENTRAS EL MOTOR ESTA ENCENDIDO
        tiempolucesfinal = millis();//mantenemos actualizado el tiempo, para una vez que se apage el motor se apage la pantalla despues de un momento

            //si el sensor detecta luz y quiere la luz encendida,
                // o se activan alarmas, o se preciona un boton la luz se mantiene encendida
        if (luzDeFondoSensor == true || A == 1 || BOTONES() != 0) {
            lcd.setBacklight(HIGH);
            estadopantalla = 0;
            apagarDespuesDeAlarma = millis(); //siempre que la luz este encendida se mantendra actualizado millis, ya sea que se activo por una alarma por un boton o por el sensor de luz

        }
        else {
            //si no hay ninguna alarma activa,
                //ningun boton se ha presionado,
                //o la luz del sensor detecta que no hay luz en el fondo
                //apagamos la pantalla
                //ahora si la luz esta apagada, despues de que se active una alarma 
                //sera comodo que la luz se quede unos 10 segundos encendida antes que vuelva a apagarse por el sensor


            if (millis() > apagarDespuesDeAlarma + 10000) {
                lcd.setBacklight(LOW);
                estadopantalla = 1; // el estado de la pantalla ahora esta apagado
            }

        }//fin de else                    


    }
    else {//ahora si el swich esta apagado es decir en 1

             //queremos que al presionar un boton,
             //activar una alarma aunque el motor este apagado la pantalla se encienda
             //y se mantenga asi por el tiempo programado para apagarce
        if (A == 1 || BOTONES() != 0 && estadopantalla == 1) {
            // en cuanto se activa una alarma,
              // o en cuanto se presiona cualquier boton
              // y reconoce que la pantalla esta apagada,
              // entonces la enciende
            lcd.setBacklight(HIGH);
            estadopantalla = 0;// ahora vuelve a activarse y se repite el ciclo actualizando con el primer if el tiempo actual y esperando a que el motor se apage
            tiempolucesfinal = millis();//mantenemos actualizado el tiempo, una ves que ya no se detecta la alarma o un boton, debera esperar el tiempo definido para volver a apagarla  
        }

        //corroboramos que la pantalla este encendida, y checamos que haya pasado el tiempo programado desde que se apago el swich, entonces apagamos la pantalla
        if (estadopantalla == 0 && millis() > tiempolucesfinal + tiempoluces) {//si elswich esta en posicion de apagado sige con el programa hasta que el tiempo del programa sea mayor a el ultimo tiempo tomado cuando el motor estaba encendido y le suma el tiempo que quiera el usuario 
            lcd.setBacklight(LOW); //una vez se cumple el tiempo programado se apaga la luz
            estadopantalla = 1; // el estado de la pantalla ahora esta apagado
        }

    }


    //CODIGO ANTES DEL SENSOR DE LUZ, SOLAMENTE APAGA LA PANTALLA DESPUES DE QUE TRANSCURRE UN CIERTO TIEMPO DESDE QUE EL MOTOR SE APAGO

    //    if (swich==0 && estadopantalla==0){
    //       tiempolucesfinal=millis();
    //   }//si el swich esta encendido y la pantalla tambien entonces solo actualizamos con el tiempo actual del programa

    //if (swich==1 && estadopantalla==0 && millis() > tiempolucesfinal + tiempoluces){//si elswich esta en posicion de apagado sige con el programa hasta que el tiempo del programa sea mayor a el ultimo tiempo tomado cuando el motor estaba encendido y le suma el tiempo que quiera el usuario 
    //    lcd.setBacklight(LOW); //una vez se cumple el tiempo programado se apaga la luz
    //    estadopantalla=1; // el estado de la pantalla ahora esta apagado
    //}

    //if(swich==1 && estadopantalla==1){
    //    tiempolucesfinal=millis();
    //}//motor y pantalla estan apagados, mantenemos la variable actualizada con el tiempo del programa

    //if(swich==0||A==1 ||BOTONES()!=0&& estadopantalla==1){// en ese momento se gira el swich a la posicion de encendido o en cuanto se activa una alarma, o en cuanto se presiona cualquier boton y reconoce que la pantalla esta apagada, entonces la enciende
    //    lcd.setBacklight(HIGH);
    //    estadopantalla=0;// ahora vuelve a activarse y se repite el ciclo actualizando con el primer if el tiempo actual y esperando a que el motor se apage  
    //}



}//Final de void Loop 



bool bloqueoDeMotorRapidoFernan() {

    byte tiemporegresivo;// el conteo que ira decrementando cada que pase 1 segundo
    unsigned long tr = 0;//variable que almacena el tiempo en millis para el conteo regresivo del menu
    String codigoIngresado = "";

    tiemporegresivo = 8;


    lcd.clear();

    lcd.print("BLOQUEO DE MOTOR");
    lcd.setCursor(0, 1);
    lcd.print("Ingrese codigo");
    AUDIO_EXITO();
    delay(1500);
    lcd.clear();


    while (true) {



        //retardo para pulsaciones
        if (millis() > retardo + 400) {

            switch (BOTONES()) {


            case BOTON_1_ACEPTAR:

                codigoIngresado += "1";
                retardo = millis();
                lcd.setCursor(0, 0);lcd.print("tiene " + String(tiemporegresivo) + "seg. ");
                lcd.setCursor(0, 1);lcd.print(codigoIngresado);
                AUDIO_BEEP();
                break;

            case BOTON_2_ABAJO:

                codigoIngresado += "2";
                retardo = millis();
                lcd.setCursor(0, 0);lcd.print("tiene " + String(tiemporegresivo) + "seg. ");
                lcd.setCursor(0, 1);lcd.print(codigoIngresado);
                AUDIO_BEEP();


                break;



            case BOTON_3_ARRIBA:
                codigoIngresado += "3";
                retardo = millis();
                lcd.setCursor(0, 0);lcd.print("tiene " + String(tiemporegresivo) + "seg. ");
                lcd.setCursor(0, 1);lcd.print(codigoIngresado);
                AUDIO_BEEP();

                break;


            case BOTON_4_SALIR:
                codigoIngresado += "4";
                retardo = millis();
                lcd.setCursor(0, 0);lcd.print("tiene " + String(tiemporegresivo) + "seg. ");
                lcd.setCursor(0, 1);lcd.print(codigoIngresado);
                AUDIO_BEEP();
                break;



            }
        }

        if (millis() > tr + 1000) {
            tiemporegresivo--;
            tr = millis();
            lcd.setCursor(0, 0);lcd.print("tiene " + String(tiemporegresivo) + "seg. ");
            lcd.setCursor(0, 1);lcd.print(codigoIngresado);
             //reiniciamos el wach dog
            // hacemos que cada que pase un segundo redusca 1 valor a tiemporegresivo
            resetWDTexterno();
        }

        if (tiemporegresivo == 0) {
            if (codigoIngresado == "4234") {
                lcd.clear();



                if (bloqueoRapidoMotor) {
                    bloqueoRapidoMotor = false;
                    EEPROM.put(26, bloqueoRapidoMotor);
                    lcd.print("El motor se ha");
                    lcd.setCursor(0, 1);
                    lcd.print("DESACTIVADO");

                }
                else {
                    bloqueoRapidoMotor = true;
                    EEPROM.put(26, bloqueoRapidoMotor);
                    lcd.print("El motor se ha");
                    lcd.setCursor(0, 1);
                    lcd.print("ACTIVADO");
                }
                AUDIO_EXITO();
                delay(1000);

                return true;
            }
            else {
                lcd.clear();
                lcd.print("El codigo es");
                lcd.setCursor(0, 1);
                lcd.print("incorrecto");
                AUDIO_ERROR();
                delay(1000);
                return false;

            }
            //una vez que tiempo regresivo sea igual a 0 comparamos los password
        }

    }//fin de while


}





bool ingresaAlMenu() {
    byte tiemporegresivo;// el conteo que ira decrementando cada que pase 1 segundo
    unsigned long tr = 0;//variable que almacena el tiempo en millis para el conteo regresivo del menu
    String codigoIngresado = "";

    tiemporegresivo = 20;

    randomSeed(millis());
    int randNumber = random(0, 150); //obtendremos un numero aleatorio del 0 al 99 el tamaño del array de los codigos    

     //vamos a combinar la apertura de la caja fuerte y el ingreso al menu en el mismo metodo, aqui mostramos el codigo variable a ingresar
     //y si concuerda ingresara a abrir la caja fuerte, pero si el numero ingresado concuerda con la contraseña para ingresar al menu entonces entramos al menu


    lcd.clear();
    lcd.print("INGRESE COD: " + randNumber);

    while (true) {



        //retardo para pulsaciones
        if (millis() > retardo + 400) {

            switch (BOTONES()) {


            case BOTON_1_ACEPTAR:

                codigoIngresado += "1";
                retardo = millis();
                lcd.setCursor(0, 0);lcd.print("COD:" + String(randNumber) + " " + String(tiemporegresivo) + "seg.");
                lcd.setCursor(0, 1);lcd.print(codigoIngresado);
                AUDIO_BEEP();
                break;

            case BOTON_2_ABAJO:

                codigoIngresado += "2";
                retardo = millis();
                lcd.setCursor(0, 0);lcd.print("COD:" + String(randNumber) + " " + String(tiemporegresivo) + "seg.");
                lcd.setCursor(0, 1);lcd.print(codigoIngresado);
                AUDIO_BEEP();


                break;



            case BOTON_3_ARRIBA:
                codigoIngresado += "3";
                retardo = millis();
                lcd.setCursor(0, 0);lcd.print("COD:" + String(randNumber) + " " + String(tiemporegresivo) + "seg.");
                lcd.setCursor(0, 1);lcd.print(codigoIngresado);
                AUDIO_BEEP();

                break;


            case BOTON_4_SALIR:
                codigoIngresado += "4";
                retardo = millis();
                lcd.setCursor(0, 0);lcd.print("COD:" + String(randNumber) + " " + String(tiemporegresivo) + "seg.");
                lcd.setCursor(0, 1);lcd.print(codigoIngresado);
                AUDIO_BEEP();
                break;



            }
        }

        if (millis() > tr + 1000) {
            tiemporegresivo--;
            tr = millis();
            lcd.setCursor(0, 0);lcd.print("COD:" + String(randNumber) + " " + String(tiemporegresivo) + "seg.");
            lcd.setCursor(0, 1);lcd.print(codigoIngresado);
             //reiniciamos el wach dog
            // hacemos que cada que pase un segundo redusca 1 valor a tiemporegresivo
            resetWDTexterno();
        }

        if (tiemporegresivo == 0) {
            if (codigoIngresado == "42341432") {
                lcd.clear();
                lcd.print("CODIGO CORRECTO");
                lcd.setCursor(0, 1);
                lcd.print("BIENVENIDO");
                AUDIO_EXITO();
                resetWDTexterno();
                delay(1000);
                return true;
            }
            else if (generaPasswordVariable(randNumber) == codigoIngresado.toInt()) {

                lcd.clear();
                lcd.setCursor(0, 1);lcd.print("Abriendo Caja");
                digitalWrite(pinCajaSeguridad, HIGH);
                AUDIO_EXITO();
                resetWDTexterno();
                delay(700);
                resetWDTexterno();
                digitalWrite(pinCajaSeguridad, LOW);
                return false;


            }
            else {

                lcd.clear();
                lcd.print("El codigo es");
                lcd.setCursor(0, 1);
                lcd.print("incorrecto");
                AUDIO_ERROR();
                resetWDTexterno();
                delay(800);
                return false;

            }
            //una vez que tiempo regresivo sea igual a 0 comparamos los password
        }






    }//fin de while


}


bool menuPrincipal() {
    //CONFIGURAR________________________________________
    byte lugar = 1;// almacena un conteo de +1 cuando se preciona el boton1 y se reinicia a 1 dependiendo el numero de opciones
    bool submenu = 0;// 1 para cuando estamos dentro de un submenu 0 cuando estamos fuer de un submenu
    unsigned long retardoBotones = 0;


    while (true) {//entramos a un bucle infinito

       // AUMENTAR LA VARIABLE LUGAR DEPENDIENDO DEL BOTON

        if (millis() > retardoBotones + 250) {//retardo entre clics

            switch (BOTONES()) {

            case BOTON_3_ARRIBA:
                lugar++;
                retardoBotones = millis();
                AUDIO_BEEP();

                if (lugar > 11) {
                    lugar = 11;
                    AUDIO_ERROR();
                } // si lugar aumenta a mas de 4 regresa a 4, sirve para fijar los limites de opciones del menu



                break;


            case BOTON_2_ABAJO:
                lugar--;
                retardoBotones = millis();
                AUDIO_BEEP();

                if (lugar < 1) {
                    lugar = 1;
                    AUDIO_ERROR();
                }

                break;


            case BOTON_1_ACEPTAR:


                break;




            case BOTON_4_SALIR:
                retardoBotones = millis();
                AUDIO_BEEP();
                lcd.clear();
                lcd.print("SALIENDO DE MENU");
                delay(2000);
                return false;

            }

        }


        //FIN DE AUMENTAR LA VARAIBLE
           //__________________________________________________________________________________________________________________________





               //MENU PRINCIPAL 1 REVOLUCIONES DE CORTE escribimos en la EEPROM en la direccion 0 un dato byte despues escribiremos en la direccion 1   
        switch (lugar) {

            //opcion 1 
        case 1:
            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("1.RPM MAXIMAS"));
                actualizarimagen = millis();
            }

            //SUB MENU 1 SUBIR Y BAJAR LAS REVOLUCIONES DE CORTE guardamos en la eprom la variable CORTE que es tipo byte solo usa un byte de la eprom la almacenamos en direccion 0
            //lo siguiente que se quiera guardar sera en la direccion 1 

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos afuera de un submenu     

                  //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) { //si funcion botones retorna 50 es decir que se presion el boton arriba
                    if (millis() > retardo + 400) {
                        corterpm += 125;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }

                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        corterpm -= 125;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }

                if (corterpm < 2000) {
                    corterpm = 2000;
                    AUDIO_ERROR();
                }

                if (corterpm > 5000) {
                    corterpm = 5000;
                    AUDIO_ERROR();
                }




                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        retardo = millis();
                        submenu = 0;
                        EEPROM.put(0, corterpm);
                        AUDIO_GUARDAR();
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(0, corterpm);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                    //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones 
                     
                    resetWDTexterno();
                    lcd.clear();
                    lcd.print("LIMITE: " + String(corterpm) + "rpm");
                    lcd.setCursor(0, 1);
                    lcd.print("Velocidad: " + String(corterpm / 30) + "km/hr");
                    actualizarimagen = millis();
                }
            }//acaba while de Submenu 1
            break;
            //__________________________________________________________________________________________________________________________








            //OPCION DE MENU 2
            /*case 2:
                if(millis()>actualizarimagen+500){//frecuencia con la que se actualiza la imagen del meno de opciones
                     
                    lcd.clear();
                    lcd.print(F("2.Calibrar RPM"));
                    actualizarimagen=millis();
                }

                //SUB MENU 2     CALIBRAR REVOLUCIONES ACTUALES
                //SUB MENU  SUBIR Y BAJAR variable ajuste, guardamos en la EEPROM un numero tipo float en la posicion 1 como untiliza 4 bites la sigiente variable a escribir la pondremos en 5

                if (BOTONES()==30){
                   submenu=1;
                   retardo=millis();
                   AUDIO_BEEP();
                 } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

                while(submenu==1){//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno


                    //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                     if (BOTONES()==50){ //si funcion botones retorna 50 es decir que se presion el boton arriba
                         if (millis()>retardo+400){
                             ajuste+=0.1;
                             retardo=millis();
                             AUDIO_BEEP();
                           } //condicional para aumento de variable cada 400ms aunque el boton este precionado
                    }

                   if (BOTONES()==40){ //si se preciona el boton abajo
                       if(millis()>retardo+400){
                           ajuste-=0.1;
                           retardo=millis();
                           AUDIO_BEEP();
                         }
                   }

                   if (ajuste<0.1){
                       ajuste=0.1;
                       AUDIO_ERROR();
                     }



                   if (BOTONES()==30){ // si se presiona aceptar
                       if(millis()>retardo+400){
                           EEPROM.put(1,ajuste);
                           retardo=millis();
                           submenu=0;
                           AUDIO_GUARDAR();
                         } //añadimos submeno=0 para salir al menu principal despues de guardar
                   }

                   if (BOTONES()==20){ // si se presiona salir
                       if(millis()>retardo+400){
                           submenu=0;
                           retardo=millis();
                           EEPROM.get(1,ajuste);
                           AUDIO_BEEP();
                           lcd.clear();
                           lcd.print("SALIENDO");
                           delay(500);
                         } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                       //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior
                   }
                   //FIN DE LECTURA DE BOTONES


                    //MOSTRAR EN LA PANTALLA
                    if(millis()>actualizarimagen+500){//frecuencia con la que se actualiza la imagen del menu de opciones
                         
                        lcd.clear();
                        lcd.print("RPM:"+String(rpm,0));
                        lcd.setCursor(0,1);
                        lcd.print("VALOR:"+String(ajuste));
                        actualizarimagen=millis();
                    }
                }//acaba while de submenu 2
            break;
            //__________________________________________________________________________________________________________________________
            */






            //opcion menu 3
        case 2:
            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("2.ALARMA DE"));
                lcd.setCursor(0, 1);
                lcd.print(F("TEMPERATURA"));
                actualizarimagen = millis();
            }

            //SUB MENU 3  LIMITE T    
            //SUB MENU  SUBIR Y BAJAR variable limiteT, guardamos en la EEPROM un numero tipo byte en la posicion 5 como untiliza 1 bites la sigiente variable a escribir la pondremos en 6   

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno

                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) { //si funcion botones retorna 50 es decir que se presion el boton arriba
                    if (millis() > retardo + 400) {
                        limiteT += 1;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }

                if (limiteT > 120) {
                    limiteT = 120;
                    AUDIO_ERROR();
                }//LIMITES PARA EVITAR PASAR DE CIERTO NUMERO LA VARIABLE



                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        limiteT -= 1;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }

                if (limiteT < 70) {
                    limiteT = 70;
                    AUDIO_ERROR();
                }//LIMITES PARA EVITAR PASAR DE CIERTO NUMERO LA VARIABLE

                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        EEPROM.put(6, limiteT);
                        retardo = millis();
                        submenu = 0;
                        AUDIO_GUARDAR();
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(6, limiteT);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    lcd.clear();
                    lcd.setCursor(1, 0);
                    lcd.print("Alarma a: " + String(limiteT) + "C");
                    actualizarimagen = millis();
                }
            }//acaba while de submenu 3       
            break;
            //__________________________________________________________________________________________________________________________






           // OPCION MENU 4 CORTE TEMPERATURA
        case 3:
            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("3.CORTE DE"));
                lcd.setCursor(0, 1);
                lcd.print(F("TEMPERATURA"));
                actualizarimagen = millis();
            }
            //SUB MENU 4     
            //SUB MENU  SUBIR Y BAJAR variable limiteTC, guardamos en la EEPROM un numero tipo byte en la posicion 6 como untiliza 1 bites la sigiente variable a escribir la pondremos en 7   

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno


                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) { //si funcion botones retorna 50 es decir que se presion el boton arriba
                    if (millis() > retardo + 400) {
                        limiteTC += 1;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }

                if (limiteTC > 120) {
                    limiteTC = 120;
                    AUDIO_ERROR();
                }//LIMITES PARA EVITAR PASAR DE CIERTO NUMERO LA VARIABLE

                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        limiteTC -= 1;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }
                if (limiteTC <= limiteT) {
                    limiteTC = limiteT + 1;
                    AUDIO_ERROR();
                    lcd.clear();
                    lcd.print("Temp Corte menor");
                    lcd.setCursor(0, 1);
                    lcd.print("A la de Alarma");
                    delay(2000);
                }//Evitamos que la temperatura para el corte sea menor a la temperatura de la alarma e iniciamos apartir de la temperatura para la alarma  

                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        EEPROM.put(7, limiteTC);
                        retardo = millis();
                        submenu = 0;
                        AUDIO_GUARDAR();
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(7, limiteTC);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    lcd.clear();
                    lcd.setCursor(1, 0);
                    lcd.print("Corte a: " + String(limiteTC) + "C");
                    actualizarimagen = millis();
                }
            }//acaba while de submenu 4
            break;
            //__________________________________________________________________________________________________________________________








            //OPCION MENU 5 ajusteT
        case 4:
            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("4.CALIBRAR LA"));
                lcd.setCursor(0, 1);
                lcd.print(F("TEMPERATURA"));
                actualizarimagen = millis();
            }
            //SUB MENU 5     
            //SUB MENU  SUBIR Y BAJAR variable ajusteT, guardamos en la EEPROM un numero tipo float en la posicion 7 como untiliza 4 bites la sigiente variable a escribir la pondremos en 11   

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno



                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) { //si funcion botones retorna 50 es decir que se presion el boton arriba
                    if (millis() > retardo + 400) {
                        ajusteT += 0.1;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }

                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        ajusteT -= 0.1;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }
                if (ajusteT < 0.1) {
                    ajusteT = 0.1;
                    AUDIO_ERROR();
                }//Evitamos que la temperatura para el corte sea menor a la temperatura de la alarma e iniciamos apartir de la temperatura para la alarma  

                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        EEPROM.put(8, ajusteT);
                        retardo = millis();
                        submenu = 0;
                        AUDIO_GUARDAR();
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(8, ajusteT);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                    //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    temperaturaD2 = ((1023 - analogRead(pinT)) / ajusteT);
                    lcd.clear();
                    lcd.print("Temp:" + String(temperaturaD2) + "C ");
                    lcd.setCursor(0, 1);
                    lcd.print(String(ajusteT));
                    actualizarimagen = millis();
                }
            }//acaba while de submenu 5
            break;
            //__________________________________________________________________________________________________________________________






       //OPCION MENU 6 ajusteBG
        case 5://ajuste de la variable ajusteG es un float usado para calibrar la lectura de voltaje de las baterias, escribimos en 15, la siguiente direccion de memoria seria 19
            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("5.CALIBRAR EL"));
                lcd.setCursor(0, 1);
                lcd.print(F("VOLTAJE"));
                actualizarimagen = millis();
            }
            //SUB MENU 7     

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno               

                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) { //si funcion botones retorna 50 es decir que se presion el boton arriba
                    if (millis() > retardo + 400) {
                        ajusteBG += 0.2;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }

                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        ajusteBG -= 0.2;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }
                if (ajusteBG < 0.2) {
                    ajusteBG = 0.2;
                    AUDIO_ERROR();
                }

                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        EEPROM.put(16, ajusteBG);
                        retardo = millis();
                        submenu = 0;
                        AUDIO_GUARDAR();
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(16, ajusteBG);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                    //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    bgD = analogRead(pinbg) / ajusteBG;
                    lcd.clear();
                    lcd.print("Voltaje:" + String(bgD) + "v ");
                    lcd.setCursor(0, 1);
                    lcd.print(String(ajusteBG));
                    actualizarimagen = millis();
                }
            }//acaba while de submenu 7
            break;
            //__________________________________________________________________________________________________________________________


        case 6://ajuste de la variable ajusteG es un float usado para calibrar la lectura de voltaje de las baterias, escribimos en 16, la siguiente direccion de memoria seria 20

            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("6.ALARMA DE USO"));
                lcd.setCursor(0, 1);
                lcd.print(F("DE GASOLINA"));
                actualizarimagen = millis();
            }
            //SUB MENU 7     

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno               

                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) { //si funcion botones retorna 50 es decir que se presion el boton arriba
                    if (millis() > retardo + 400) {
                        alarmaBifuelActiva = true;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }

                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        alarmaBifuelActiva = false;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }


                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        EEPROM.put(20, alarmaBifuelActiva);
                        retardo = millis();
                        submenu = 0;
                        AUDIO_GUARDAR();
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(20, alarmaBifuelActiva);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                    //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    lcd.clear();
                    lcd.print("La Alarma esta:");
                    lcd.setCursor(0, 1);

                    if (alarmaBifuelActiva) {
                        lcd.print("ACTIVADA");
                    }
                    else {
                        lcd.print("DESACTIVADA");
                    }

                    actualizarimagen = millis();
                }
            }//acaba while de submenu 7
            break;
            //__________________________________________________________________________________________________________________________






        case 7://Ajuste del tiempo de ahorro de energia

            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("7.AHORRO"));
                lcd.setCursor(0, 1);
                lcd.print(F("DE ENERGIA"));
                actualizarimagen = millis();
            }
            //SUB MENU 7     

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno               

                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) {
                    if (millis() > retardo + 400) {
                        enteroLimiteAhorro += 1;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }


                if (enteroLimiteAhorro > 255) {
                    enteroLimiteAhorro = 255;
                    AUDIO_ERROR();
                }//LIMITES PARA EVITAR PASAR DE CIERTO NUMERO LA VARIABLE

                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        enteroLimiteAhorro -= 1;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }

                if (enteroLimiteAhorro < 2) {
                    enteroLimiteAhorro = 2;
                    AUDIO_ERROR();
                }//LIMITES PARA EVITAR PASAR DE CIERTO NUMERO LA VARIABLE


                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        EEPROM.put(22, enteroLimiteAhorro);
                        retardo = millis();
                        submenu = 0;
                        AUDIO_GUARDAR();
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(22, enteroLimiteAhorro);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                    //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    lcd.clear();
                    lcd.print("Esperar para");
                    lcd.setCursor(0, 1);
                    lcd.print("apagado: " + String(enteroLimiteAhorro * 30) + "min");

                    actualizarimagen = millis();
                }
            }//acaba while de submenu 7
            break;
            //__________________________________________________________________________________________________________________________






        case 8://Ajuste del limite sensor luz

            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("8.Sensor Luz"));
                lcd.setCursor(0, 1);
                lcd.print(F("Apagado Pantalla"));
                actualizarimagen = millis();
            }
            //SUB MENU 7     

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno               

                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) {
                    if (millis() > retardo + 400) {
                        limiteInferiorSensorLuz += 10;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }


                if (limiteInferiorSensorLuz > 800) {
                    limiteInferiorSensorLuz = 800;
                    AUDIO_ERROR();
                }//LIMITES PARA EVITAR PASAR DE CIERTO NUMERO LA VARIABLE

                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        limiteInferiorSensorLuz -= 10;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }

                if (limiteInferiorSensorLuz < 10) {
                    limiteInferiorSensorLuz = 10;
                    AUDIO_ERROR();
                }//LIMITES PARA EVITAR PASAR DE CIERTO NUMERO LA VARIABLE


                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        EEPROM.put(23, limiteInferiorSensorLuz);
                        retardo = millis();
                        submenu = 0;
                        AUDIO_GUARDAR();
                        lcd.setBacklight(true);
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(23, limiteInferiorSensorLuz);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        lcd.setBacklight(true);
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                    //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    lcd.clear();
                    lcd.print("Limite luz: " + String(limiteInferiorSensorLuz));
                    lcd.setCursor(0, 1);
                    lcd.print("Luz Actual: " + String(analogRead(sensorLuz)));

                    if (analogRead(sensorLuz) < limiteInferiorSensorLuz) {
                        lcd.setBacklight(false);
                    }
                    else {
                        lcd.setBacklight(true);
                    }

                    actualizarimagen = millis();
                }
            }//acaba while de submenu 7
            break;
            //__________________________________________________________________________________________________________________________



        case 9://ajuste de la variable ajusteG es un float usado para calibrar la lectura de voltaje de las baterias, escribimos en 16, la siguiente direccion de memoria seria 20

            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("9.ALARMAS VOLTA"));
                lcd.setCursor(0, 1);
                lcd.print(F("Y ALTERNADOR"));
                actualizarimagen = millis();
            }
            //SUB MENU 7     

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno               

                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU
                if (BOTONES() == BOTON_3_ARRIBA) { //si funcion botones retorna 50 es decir que se presion el boton arriba
                    if (millis() > retardo + 400) {
                        alarmasBateriaActivas = true;
                        retardo = millis();
                        AUDIO_BEEP();
                    } //condicional para aumento de variable cada 400ms aunque el boton este precionado 
                }

                if (BOTONES() == BOTON_2_ABAJO) { //si se preciona el boton abajo
                    if (millis() > retardo + 400) {
                        alarmasBateriaActivas = false;
                        retardo = millis();
                        AUDIO_BEEP();
                    }
                }


                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar
                    if (millis() > retardo + 400) {
                        EEPROM.put(25, alarmasBateriaActivas);
                        retardo = millis();
                        submenu = 0;
                        AUDIO_GUARDAR();
                    } //añadimos submeno=0 para salir al menu principal despues de guardar
                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        EEPROM.get(25, alarmasBateriaActivas);
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    } //aunque se alla cambiado la variable dentro del menu pero precionas cancelar volvemos a leer el valor
                    //almacenado el la eeprom, porque si no añadiamos ese comando modificabas la variable auqnue le dieras cancelar, auqneu esta no se guardaria en la eprom y al reiniciar cargaria la anterior 
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    lcd.clear();
                    lcd.print("Alarmas estan:");
                    lcd.setCursor(0, 1);

                    if (alarmasBateriaActivas) {
                        lcd.print("ACTIVADA");
                    }
                    else {
                        lcd.print("DESACTIVADA");
                    }

                    actualizarimagen = millis();
                }
            }//acaba while de submenu 7
            break;







        case 10:
            if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del meno de opciones
                 
                resetWDTexterno();
                lcd.clear();
                lcd.print(F("10.RESTABLECER"));
                lcd.setCursor(0, 1);
                lcd.print(F("LOS VALORES"));
                actualizarimagen = millis();
            }
            //SUB MENU 9     
            //SUB MENU  ESCRIBIR EN LA MEMORIA EPPRON LOS DATOS POR PRIMERA VEZ   

            if (BOTONES() == BOTON_1_ACEPTAR) {
                submenu = 1;
                retardo = millis();
                AUDIO_BEEP();
            } // PRECIONAMOS ACEPTARponemos retardo porque en cuanto precionabas y entraba al while alcansaba a ejecutar la orden del boton dentro del while

            while (submenu == 1) {//entramos al submenu solo si se preciona el boton aceptar y estamos fuera de un submeno     

                //LECTURA Y FUNCION DE LOS BOTONES DENTRO DEL SUBMENU

                if (BOTONES() == BOTON_1_ACEPTAR) { // si se presiona aceptar asignamos las variables y las escribimos en la memoria
                    if (millis() > retardo + 400) {
                        corterpm = 3250;
                        ajuste = 6.7;
                        limiteT = 98;
                        limiteTC = 103;
                        ajusteT = 8.8;//descomentar para activar la varaible ajusteT para el sensor analogo del vehiculo
                        ajusteBG = 62.3;
                        alarmaBifuelActiva = true;
                        contadorAhorroEnergia = 0; //reiniciamos el contador y lo guardamos en la memoria
                        enteroLimiteAhorro = 28; //iniciamos tiempo limite en 14 horas o 840 minutos
                        limiteInferiorSensorLuz = 180; //menor a este valor se apaga la luz de la pantalla
                        alarmasBateriaActivas = true;
                        puenteVentilador = false;
                        temperaturaEstimada = 0;


                        EEPROM.put(0, corterpm); //guarda int 2 posiciones de la memoria del 0 al 1 siguiente lugar 2
                        EEPROM.put(2, ajuste);   //guarda float 4 posiciones del 2 al 5 siguiente lugar 6
                        EEPROM.put(6, limiteT);  //guarda byte 1 posicion del 6 al 6 siguiente lugar 7
                        EEPROM.put(7, limiteTC); //guarda byte 1 posicion del 7 al 7 siguiente lugar 8
                        EEPROM.put(8, ajusteT);  //guarda float 4 posiciones del 8 al 11 siguiente lugar 12
                        EEPROM.put(16, ajusteBG);//guarda float 4 posiciones del 16 al 19 siguiente lugar 20
                        EEPROM.put(20, alarmaBifuelActiva);//guarda bool 1 posicision del 20 al 20 siguiente posicion 21
                        EEPROM.put(21, contadorAhorroEnergia);//guarda un byte en la posicion 21 a 21 siguiente posicion 22
                        EEPROM.put(22, enteroLimiteAhorro);//guarda un byte en la posicion 22 a 22 siguiente posicion 23
                        EEPROM.put(23, limiteInferiorSensorLuz);//guarda un int en la posicion 23 a 24 siguiente posicion 25
                        EEPROM.put(25, alarmasBateriaActivas);//guarda un bool en la posicion 25 a 25 siguiente posicion 26
                        EEPROM.put(26, bloqueoRapidoMotor);//guarda un bool en la posicion 26 a 26 siguiente posicion 27
                        EEPROM.put(27, puenteVentilador);//guarda un bool en la posicion 27 a 27 siguiente posicion 28
                        EEPROM.put(28, temperaturaEstimada);//guarda un byte en la posicion 28 a 28 siguiente posicion 29





                        AUDIO_GUARDAR();
                        submenu = 0;
                    }

                }

                if (BOTONES() == BOTON_4_SALIR) { // si se presiona salir
                    if (millis() > retardo + 400) {
                        submenu = 0;
                        retardo = millis();
                        AUDIO_BEEP();
                        lcd.clear();
                        lcd.print("SALIENDO");
                        delay(500);
                    }
                }
                //FIN DE LECTURA DE BOTONES  


                 //MOSTRAR EN LA PANTALLA 
                if (millis() > actualizarimagen + 500) {//frecuencia con la que se actualiza la imagen del menu de opciones
                     
                    resetWDTexterno();
                    lcd.clear();
                    lcd.print("precione ACEPTAR");
                    lcd.setCursor(0, 1);
                    lcd.print("para reiniciar");
                    actualizarimagen = millis();
                }
            }//acaba while de submenu 9
            break;



            //______________________________________________________________________________
        case 11: //mostramos la version del software y reiniciamos en automatico pasados 4 segundos
            if (millis() > actualizarimagen + 250) {//frecuencia con la que se actualiza la imagen del meno de opciones
                //  
                lcd.clear();
                lcd.print("11.-soft: " + VERSION_SOFTEARE);


                actualizarimagen = millis();
            }
            break;



        }//Final de switch


    }//final de En Menu
}






bool abreCajaFuerte() {
    //sacamos un nuevo password enviandole un numero aleatorio del 0 al 99
    AUDIO_EXITO();
    digitalWrite(bomba, LOW);

    randomSeed(millis());
    int randNumber = random(0, 150); //obtendremos un numero aleatorio del 0 al 99 el tamaño del array de los codigos

    lcd.clear();
    lcd.setCursor(0, 0);lcd.print("Codigo Num:" + String(randNumber));
    //lcd.setCursor(0,1);lcd.print("Pasword: " + String(generaPasswordVariable(randNumber)));
    String passwordIngresado = "";
    unsigned long tiempoEntreClics = millis();

    while (swich == 0) {
        swich = digitalRead(pinswich);
         
        resetWDTexterno();


        if (millis() > tiempoEntreClics + 500) {



            switch (BOTONES()) {



            case BOTON_1_ACEPTAR: //BOTON NUMERO 1


                passwordIngresado += "1";
                lcd.setCursor(0, 1);lcd.print(passwordIngresado);
                tiempoEntreClics = millis();
                AUDIO_BEEP();



                break;
            case BOTON_2_ABAJO: //BOTON NUMERO 2
              // do something


                passwordIngresado += "2";
                lcd.setCursor(0, 1);lcd.print(passwordIngresado);
                tiempoEntreClics = millis();
                AUDIO_BEEP();

                break;

            case BOTON_3_ARRIBA: //BOTON NUMERO 3
            // do something


                passwordIngresado += "3";
                lcd.setCursor(0, 1);lcd.print(passwordIngresado);
                tiempoEntreClics = millis();
                AUDIO_BEEP();

                break;

            case BOTON_4_SALIR: //BOTON NUMERO 4
            // do something

                passwordIngresado += "4";
                lcd.setCursor(0, 1);lcd.print(passwordIngresado);
                tiempoEntreClics = millis();
                AUDIO_BEEP();

                break;

            }


        }
    }//fin de while

    if (generaPasswordVariable(randNumber) == passwordIngresado.toInt()) {
        lcd.clear();
        lcd.setCursor(0, 1);lcd.print("Abriendo Caja");
        digitalWrite(pinCajaSeguridad, HIGH);
        resetWDTexterno();
        AUDIO_EXITO();
        delay(800);
        resetWDTexterno();
        digitalWrite(pinCajaSeguridad, LOW);
    }
    else {
        lcd.clear();
        lcd.setCursor(0, 0);lcd.print("Codigo");
        lcd.setCursor(0, 1);lcd.print("Incorrecto");
        AUDIO_ERROR();
        delay(800);

    }




}

int generaPasswordVariable(int randNumber) {

    int  password[150] = { 1232,
                            3322,
                            4334,
                            4324,
                            1332,
                            4314,
                            2432,
                            2142,
                            4342,
                            2132,
                            2224,
                            4114,
                            1343,
                            3224,
                            4142,
                            2114,
                            4221,
                            3332,
                            2434,
                            1134,
                            4411,
                            3131,
                            1342,
                            4212,
                            1444,
                            3142,
                            3412,
                            1411,
                            2133,
                            2142,
                            1212,
                            1333,
                            3141,
                            2414,
                            4142,
                            2421,
                            1234,
                            4241,
                            2444,
                            4421,
                            3214,
                            3121,
                            4231,
                            3411,
                            3311,
                            2234,
                            3143,
                            3341,
                            2121,
                            2114,
                            4132,
                            4212,
                            3122,
                            1224,
                            3141,
                            4123,
                            4123,
                            1332,
                            1332,
                            2233,
                            4443,
                            2132,
                            4411,
                            2343,
                            1113,
                            3142,
                            1243,
                            1341,
                            2224,
                            1442,
                            4112,
                            3142,
                            3214,
                            2234,
                            3442,
                            4443,
                            2123,
                            3313,
                            2341,
                            2312,
                            4144,
                            3334,
                            2333,
                            3233,
                            1241,
                            3141,
                            3234,
                            1413,
                            1213,
                            2242,
                            3343,
                            3434,
                            1424,
                            2141,
                            1212,
                            1133,
                            4132,
                            4122,
                            3442,
                            4314,
                            3124,
                            2431,
                            4112,
                            2243,
                            2312,
                            2242,
                            3243,
                            1212,
                            1323,
                            4323,
                            3311,
                            4412,
                            2123,
                            3222,
                            4214,
                            3234,
                            1123,
                            3324,
                            3433,
                            2311,
                            4232,
                            3113,
                            4244,
                            4332,
                            4111,
                            3114,
                            4133,
                            2414,
                            3444,
                            4414,
                            2341,
                            3131,
                            4113,
                            3343,
                            1441,
                            1214,
                            4312,
                            3213,
                            4431,
                            1323,
                            3444,
                            3424,
                            1334,
                            2121,
                            4342,
                            1221,
                            3141,
                            2231,
                            3343,
                            4433
    };



    //retornamos el password seleccionado del array
    return password[randNumber];

}




void funcionInterrupcion() {
    numInterrupt++;

}








int BOTONES() {

    //EN LOS NUEVOIS CIRCUITOS AL PARECER ME EQUIVOUE PUSE EL ORDEN MAL DE LOS BOTONES
    //EN LOS CIRCUITOS ANTERIORES QUE SE DEBE DE CONECTAR UN TECLADO EXTERNO SON LAS LINEAS COMENTADAS
    //Y EN LOS QUE LOS BOTONES VIENEN INTEGRADOS SON LOS DESCOMENTADOS
    //COMENTAR Y DESCMENTAR LAS FUNCIONES DEPENDIENDO DEL QUE SE REQUIERA CARGAR PARA QUE FUNCIONEN BIEN LOS BOTONES
    int botonera;
    botonera = analogRead(b);
    // definicion de los margenes entregados por analogread para reconocer cada boton calibrado a 5v de alimentacion. 



    //FUNCIONES DE LOS BOTONES DE CIRCUITOS NUEVOS QUE INCLUYEN LOS BOTONES SOLDADOS
    if (botonera > 900 && botonera < 1024) {
        return BOTON_1_ACEPTAR;
    }// 3 ARRIBA retonrna 50 cuando se preciona   de 399 a 420  15k a 5v y 10k a tierra  369 resistencia de 20k

    if (botonera > 280 && botonera < 400) {
        return BOTON_3_ARRIBA;
    }// 4 SALIR y guardar retorna 20 al precionarce   180 a 194   47k a5v y 10k a tierra

    if (botonera > 120 && botonera < 220) {
        return BOTON_4_SALIR;
    } // 2 ABAJO retorna 40 cuando se preciona  512 a 500 resistencia de 10k y 10k a tierra 

    if (botonera > 460 && botonera < 550) {
        return BOTON_2_ABAJO;
    }//1 ACEPTAR retorna 50 cuando se preciona valor con resistencia de 1k a 5v y 10k a tierra=915 a 934   

    else {
        return (0);
    } // si no se presiona ningun boton retorna 0





         //FUNICONES de botones para los teclados azules
   /*
   if (botonera > 900 && botonera < 1024){
       return BOTON_3_ARRIBA;
    }// 3 ARRIBA retonrna 50 cuando se preciona   de 399 a 420  15k a 5v y 10k a tierra  369 resistencia de 20k

    if (botonera > 280 && botonera < 400){
       return BOTON_4_SALIR;
    }// 4 SALIR y guardar retorna 20 al precionarce   180 a 194   47k a5v y 10k a tierra

    if (botonera > 120 && botonera < 220){
       return BOTON_2_ABAJO;
    } // 2 ABAJO retorna 40 cuando se preciona  512 a 500 resistencia de 10k y 10k a tierra

    if (botonera > 460 && botonera < 550){
       return BOTON_1_ACEPTAR;
    }//1 ACEPTAR retorna 50 cuando se preciona valor con resistencia de 1k a 5v y 10k a tierra=915 a 934

    else{
       return (0);
    } // si no se presiona ningun boton retorna 0

    */






    //NUESTRA BOTONERA FUNCIONA CON UN SOLO PUERTO ANALOGICO CADA BOTON TIENE CONECTADO UNA RESISTENCIA QUE AL PRECIONARLO
    //HACE QUE VARIE EL VOLTAJE QUE RECIBE EL PION ANALOGICO, CADA RESISTENCIA DE CADA BOTON ES DE UN VALOR DIFERENTE
    //ENTONCES HAY LECTURAS DE VOLTAJE DIFERENTES DEPENDIENDO DEL BOTON QUE SE PRECIONE, ES UN DIVISOR DE TENCION
    //ESTO NOS PERMITE CON UN SOLO PIN DE ARDUINO LEER 4 BOTONES O MAS. YA QUE LOS TECLADOS COMUNES NECESITAN COMO 5 O MAS PINES DIGITALES
}


void resetWDTexterno() {
   if (cambioEstadoWDT) {
      digitalWrite(pinWDT, LOW);
      cambioEstadoWDT = false;
   }
   else {
      digitalWrite(pinWDT, HIGH);
      cambioEstadoWDT = true;
    }
}



void funcionLimpiezaDeRelay() {
    //esta funcion lo que hara es hacer vibrar el relevador de la bobina para tratar como un ultimo intento
    //de quitar el carbon que se forma en el contacto del relay despues de un tiempo de uso, cuando deproto el motor ya no arranca, si esto no lo resuelve se tendra que puentear el sistema
    // de todas maneras compraremos relevadores de mejor calidad como los de bomba de gasolina rosas de chevy para evitar al maximo este problema
    // entraremos a un bucle while que se quedara haceido vibrar el relevador durante un numero de veces determinado
    //salimos del bucle


    int tiempoVibracion = 0;
    int numeroDeVibraciones = 0;
    int contadorlimp = 0;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Limpiando los");
    lcd.setCursor(0, 1);
    lcd.print("Contactos  0%");
    AUDIO_EXITO();




    for (int iterador = 1; iterador <= 10; iterador++) {

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Limpiando los");
        lcd.setCursor(0, 1);
        lcd.print("Contactos  " + String(iterador * 10) + "%");

        if (iterador == 1 || iterador == 6) {
            tiempoVibracion = 50;
            numeroDeVibraciones = 100;
        }

        if (iterador == 2 || iterador == 7) {
            tiempoVibracion = 30;
            numeroDeVibraciones = 150;

        }

        if (iterador == 3 || iterador == 8) {
            tiempoVibracion = 20;
            numeroDeVibraciones = 200;
        }

        if (iterador == 4 || iterador == 9) {
            tiempoVibracion = 15;
            numeroDeVibraciones = 300;
        }

        if (iterador == 5 || iterador == 10) {
            tiempoVibracion = 10;
            numeroDeVibraciones = 350;
        }





        if (iterador <= 5) {
            //aqui activamos la limpieza del relevador de bomba
            contadorlimp = 0;
            while (contadorlimp <= numeroDeVibraciones) {
                 
                resetWDTexterno();
                digitalWrite(bomba, HIGH);
                delay(tiempoVibracion);
                digitalWrite(bomba, LOW);
                delay(tiempoVibracion);
                contadorlimp++;
            }

        }
        else {
            //las siguientes 5 veces del bucle limpiamos el rele de ventilador

            contadorlimp = 0;
            while (contadorlimp <= numeroDeVibraciones) {
                 
                resetWDTexterno();
                digitalWrite(ventilador, HIGH);
                delay(tiempoVibracion);
                digitalWrite(ventilador, LOW);
                delay(tiempoVibracion);
                contadorlimp++;
            }

        }







    }


    AUDIO_EXITO();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Limpieza ");
    lcd.setCursor(0, 1);
    lcd.print("Finalizada");
    delay(800);


}


//AUDIOS_________________________________________________________________
void AUDIO_BEEP() {
    resetWDTexterno();
    digitalWrite(buzzer, HIGH);
    delay(20);
    digitalWrite(buzzer, LOW);
}

void AUDIO_ERROR() {
    for (int i = 0;i < 2;i++) {
        resetWDTexterno();
        digitalWrite(buzzer, HIGH);
        delay(30);
        digitalWrite(buzzer, LOW);
        delay(30);
    }
    numInterrupt = 0; //ponemos en 0 el numero de interrupciones porque los delays ocacionan que al volver al flujo normal de programa el conteo de los interrup siguio aumentando y eso hace que el motor se apage, por ejemplo al avisar de alarma de temperatura, entramos al audio alarma donde le toma 1.2 segundos terminar el audio durante ese tiempo las interrupt si siguieron aumentando, si el conteo es de 16 interrup cada 250ms es decir 1000rpm, al finalisar el audio son 80 interrup porque nunca entra al codigo que reinicia el conteo de interrupt, esto le dice al sistema que el motor va a 5000rpm y apaga el motor. 

}

void AUDIO_ALARMA() {
    for (int i = 0;i < 5;i++) {
        resetWDTexterno();
        digitalWrite(buzzer, HIGH);
        delay(200);
        digitalWrite(buzzer, LOW);
        delay(30);
    }
    numInterrupt = 0;
}

void AUDIO_LIGERO() {
    for (int i = 0;i < 3;i++) {
        resetWDTexterno();
        digitalWrite(buzzer, HIGH);
        delay(100);
        digitalWrite(buzzer, LOW);
        delay(30);
    }
    numInterrupt = 0;
}


void AUDIO_MEDIO() {
    for (int i = 0;i < 4;i++) {
        resetWDTexterno();
        digitalWrite(buzzer, HIGH);
        delay(150);
        digitalWrite(buzzer, LOW);
        delay(30);
    }
    numInterrupt = 0;
}

void AUDIO_GRAVE() {
    for (int i = 0;i < 3;i++) {
        resetWDTexterno();
        digitalWrite(buzzer, HIGH);
        delay(400);
        digitalWrite(buzzer, LOW);
        delay(30);
    }
    numInterrupt = 0;
}

void AUDIO_EXITO() {
    digitalWrite(buzzer, HIGH);
    delay(300);
    digitalWrite(buzzer, LOW);
    delay(30);
    for (int i = 0;i < 2;i++) {
        resetWDTexterno();
        digitalWrite(buzzer, HIGH);
        delay(80);
        digitalWrite(buzzer, LOW);
        delay(30);
    }
    digitalWrite(buzzer, HIGH);
    delay(300);
    digitalWrite(buzzer, LOW);
    delay(30);
}

void AUDIO_GUARDAR() {
    lcd.clear();
    lcd.print(F("Guardado Exitoso"));
    
    for (int i = 0;i < 6;i++) {
        resetWDTexterno();
        digitalWrite(buzzer, HIGH); //si se escribio algo en la memoria activa el buzzer hace un tono con el buzzer
        delay(50);
        digitalWrite(buzzer, LOW);
        delay(30);
    }
    delay(800);

}



//________________________________________________________________________________
