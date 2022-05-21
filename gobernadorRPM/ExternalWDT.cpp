#include "ExternalWDT.h"


bool ExternalWDT::cambioEstadoWDT = false;
void ExternalWDT::reset()
{
    
        if (cambioEstadoWDT) {
            digitalWrite(PIN_WDT, LOW);
            cambioEstadoWDT = false;
        }
        else {
            digitalWrite(PIN_WDT, HIGH);
            cambioEstadoWDT = true;
        }
    
}
