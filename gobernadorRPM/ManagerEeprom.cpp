#include "ManagerEeprom.h"

void ManagerEeprom::setSettings()
{
	EEPROM.put(_EE_ADRESS_SETTINGS, settings);
}

void ManagerEeprom::getSettings()
{
	EEPROM.get(_EE_ADRESS_SETTINGS, settings);
}

void ManagerEeprom::setDefaultSettings()
{
	settings.thresholdRpm = 3250;
	settings.adjustRpm = 6.7;
	settings.thresholdAlmTemp = 98;
	settings.thresholdCutTemp = 103;
	settings.adjustTemp = 8.8;
	settings.adjustVoltaje = 62.3;
	settings.almBiFuel = true;
	settings.contPowerSave = 0;
	settings.thresholdPowerSave = 10;
	settings.almBatery = true;
	settings.stimateTemperature = 0;
	this->setSettings();

}
