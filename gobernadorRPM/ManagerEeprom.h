#pragma once
#include <Arduino.h>
#include <EEPROM.h>
class ManagerEeprom
{
private:
#define _EE_ADRESS_SETTINGS 0				//The initian position of the struct settings
#define _EE_SIZEOF_SETTINGS 19


	struct Settings
	{
		int thresholdRpm = 0;				//2 bytes is an int because here we are going to save the revolutions given where the engine will turn off viz.3250rpm 
		float adjustRpm = 0;				//4 bytes depend on each crankshaft sensor and adjust will be needed so the lecture given by the interrupt multiply by this number will give us the current rpm of the engine
		byte thresholdAlmTemp = 0;			//1 byte it allow us to storage from 0 to 255 because is temperature we not need more capacity, hence an engine should be 100degres maximum
		byte thresholdCutTemp = 0;			//1 byte the threshold on which the engine should be turned out incase of high temperature
		float adjustTemp = false;				//4 bytes, the analog read given from the temperature sensor divided by this value will give us the temperature
		float adjustVoltaje = 0;			//4 bytes, the same has the superiorone
		bool almBiFuel = false;				//1 byte, if we want to active the bifuel alarms
		byte contPowerSave = 0;				//1 byte, this counter will increase 1 time, each 30 minutes or acoording with the code, to determine the operative time of the sistem
		byte thresholdPowerSave = 0;		//1 byte, this counter stablish at which time the sistem should turned off by itself
		bool almBatery = false;				//1 byte, if the low battery alarms should go off
		byte stimateTemperature = 0;		//1 byte, determine the temperature that the sistem guess according with the time since the engine has been turned off
	};

public:
	Settings settings;

	/*
	Every time you change the setting structure you must call this metod to save it into the eeprom
	*/
	void setSettings();
	void getSettings();
	void setDefaultSettings();

};

static ManagerEeprom mnEprom; //we declare one eeprom class as not to declare and object

