#pragma once
#include "PinDesignation.h"
#include <Arduino.h>

//We need this class to contain the statics methods needed for the interrupt function
//because when you call the method attachInterrupt(); you will need by force an static fuction.
//also must be static, because we only most have one unique copy of the var
class Attach
{
private:
	static volatile int interrupRPM;

public:
	static void conteoRPM();
};

