#include "Attach.h"
volatile int Attach::interrupRPM = 0;
void Attach::conteoRPM()
{
	Attach::interrupRPM++;
}
