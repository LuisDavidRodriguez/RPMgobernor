#include "Screen.h"
#include <LiquidCrystal_I2C.h>





Screen::Screen(LiquidCrystal_I2C *pan) : LiquidCrystal_I2C(0, 16, 2)
{
	this->pantalla = pan;
}

Screen::Screen(uint8_t address) : LiquidCrystal_I2C (address,16,2)
{
	pantalla = new LiquidCrystal_I2C(address, 16, 2);
	//pantalla es una variable de tipo puntero es decir que guardara una direccion de la memoria donde se crea el objeto
	//por eso usamos la palabra reservada new para crear un nuevo objeto pero new ya entrega una referencia es decir una 
	//direccion de la memoria donde se creo el nuevo objeto y la direcicon se almacenara en pantalla que es un puntero
	//entocnes podremos usar ambos constructores, tanto el superior que es creando el objeto liquidCristal fuera de esta clase
	//y pasandole solo su referencia de memoria, o pasando la direccion y creando aqui el objeto liquidCristal
}

void Screen::begin()
{
	this->pantalla->init();
	this->pantalla->setBacklight(HIGH);	
}

void Screen::mainInfo(float rpm, float batteryVoltage, float temperature, byte minutes, byte seconds, String mode, bool puenteVentilador)
{
	pantalla->clear();
	pantalla->home();
	pantalla->setCursor(0, 0);
	pantalla->print(String(rpm, 0));
	pantalla->setCursor(5, 0);
	pantalla->print(String(batteryVoltage, 1) + "v");
	pantalla->setCursor(11, 0);
	pantalla->print(String(temperature, 0) + "C");
	pantalla->setCursor(0, 1);

	pantalla->print(String(minutes) + ":" + String(seconds) + " " + mode + String(puenteVentilador == true ? "  V" : ""));


}

void Screen::bloqueoInfo()
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("MOTOR APAGADO"));
	pantalla->setCursor(0, 1);
	pantalla->print(F("LLAME A KALIOPE"));
}

void Screen::almTemperature(String temp)
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("Temperatura Alta"));
	pantalla->setCursor(0, 1);
	pantalla->print(temp + "C");
	pantalla->setCursor(5, 1);
	pantalla->print(F("VENTILANDO"));
}

void Screen::almTemperatureOut(String temp)
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("MOTOR FUERA DE"));
	pantalla->setCursor(0, 1);
	pantalla->print("TEMPERATURA" + temp + "C");
}

void Screen::almLights()
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("LUCES ENCENDIDAS"));
	pantalla->setCursor(0, 1);
	pantalla->print(F("APAGALAS!!!"));
}

void Screen::almBiFuel()
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("CAMBIA A GasLP"));
	pantalla->setCursor(0, 1);
	pantalla->print(F("usas gasolina"));
}

void Screen::almFallaAlternador(String voltaje)
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("FALLA DE"));
	pantalla->setCursor(0, 1);
	pantalla->print("ALTERNADOR" + voltaje + "v");
}

void Screen::almLowBattery()
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("BATERIA BAJA"));
	pantalla->setCursor(0, 1);
	pantalla->print(F("ENCIENDA MOTOR"));
}

void Screen::almSwitch()
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("EL SWICH ESTA"));
	pantalla->setCursor(0, 1);
	pantalla->print(F("ABIERTO, APAGALO"));
}

void Screen::almPuenteVentilador()
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("DESACTIVA PUENTE"));
	pantalla->setCursor(0, 1);
	pantalla->print(F("DEL VENTILADOR!!"));
}

void Screen::almPuenteVentilador2()
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print(F("PUENTE VENTILADOR"));
	pantalla->setCursor(0, 1);
	pantalla->print(F("LLAMA A KALIOPE"));
}

void Screen::info1(int rpm, byte limT, byte limTC)
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print("C:" + String(rpm) + " " + String(rpm / 30) + "km/h");
	pantalla->setCursor(0, 1); 
	pantalla->print("AT:" + String(limT) + "c TC:" + String(limTC) + "c");


}

void Screen::info3(String bom, String ign, String lights, String swt, String vent, String gasoline, String bloqGps)
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print("M:" + bom + " I:" + ign + " L:" + lights + " SW:" + swt);
	pantalla->setCursor(0, 1); 
	pantalla->print("V:" + vent + " GLP:" + gasoline + " GPS:" + bloqGps);
}

void Screen::info2(String luz, String limLuz, String tempEst, String actTemp, String verSoft)
{
	pantalla->clear();
	pantalla->setCursor(0, 0);
	pantalla->print("Luz:" + luz + " Lim:" + limLuz);
	pantalla->setCursor(0, 1);
	pantalla->print("Ts:" + tempEst + "c " + "E:" + actTemp+ " " + verSoft);
	//mostramos la temperatura estimada que es la que el sistema calcula dependiendo del tiempo que le motor esta apagado o encendido
	//y tambien mostramos el valor de las alarmas de temperatura si es un 0 significa que el sistema ya las activo, si es un 1 significa que estan desactivadas
}
