#pragma once
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>

class Screen : public LiquidCrystal_I2C		
	//once you aplie for heritence, this class has a problem because if yo go to the LiquidCrystal_I2c you are going to see there is no a predeterminated constructor its mean one with 0 arguments
	//and you needed cause the compiler show you this error parent doesn't have a predeterminate constructor, you can either add a no arguments constructor in your parent class or call the only constructor
	// explicit in your cpp file and constructor, because in the parent class only found this constructor LiquidCrystal_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows);
	// so you need to call it explisit.
	// in the .h file everithing look the same, the problem only is in .cpp file
	// /*
	// Screen::Screen(LiquidCrystal_I2C *pan) : LiquidCrystal_I2C(0, 16, 2)
	//{
	//	this->pantalla = pan;
	//}
	//Screen::Screen(uint8_t address) : LiquidCrystal_I2C(address, 16, 2)
	//{
	//pantalla = new LiquidCrystal_I2C(address, 16, 2);
	//pantalla es una variable de tipo puntero es decir que guardara una direccion de la memoria donde se crea el objeto	
	//}
	// */
	// 
	// https://ajaxhispano.com/ask/error-de-c-el-padre-no-contiene-un-constructor-que-tome-0-argumentos-6225/
	//https://es.acervolima.com/orden-de-llamada-al-constructor-destructor-en-c/
{
private:
	LiquidCrystal_I2C *pantalla;



public: 	
	
	Screen(LiquidCrystal_I2C *pan);
	Screen(uint8_t address);

	virtual void begin();

	void mainInfo(float rpm, float batteryVoltage, float temperature, byte minutes, byte seconds, String mode, bool puenteVentilador);

	void bloqueoInfo();

	void almTemperature(String temp);
	void almTemperatureOut(String temp);
	void almLights();
	void almBiFuel();
	void almFallaAlternador(String voltaje);
	void almLowBattery();
	void almSwitch();
	void almPuenteVentilador();
	void almPuenteVentilador2();
	void info1(int rpm, byte limT, byte limTC);
	void info3(String bom, String ign, String lights, String swt, String vent, String gasoline, String bloqGps);
	void info2(String luz, String limLuz, String tempEst, String actTemp, String verSoft);
	
};

