#include "MonitorRpm.h"

void MonitorRpm::begin()
{
	//create the interruptListener, and specifi which pin will be used
	attachInterrupt(digitalPinToInterrupt(PIN_RPM), Attach::conteoRPM, FALLING);
}

void MonitorRpm::stop()
{
	detachInterrupt(digitalPinToInterrupt(PIN_RPM));
}
