#include "Audio.h"
#include "ExternalWDT.h"

void Audio::beep()
{
    ExternalWDT::reset();
    digitalWrite(PIN_BUZZER, HIGH);
    delay(20);
    digitalWrite(PIN_BUZZER, LOW);
}

void Audio::error()
{
    for (int i = 0; i < 2; i++) {
        ExternalWDT::reset();
        digitalWrite(PIN_BUZZER, HIGH);
        delay(30);
        digitalWrite(PIN_BUZZER, LOW);
        delay(30);
    }
    //numInterrupt = 0; //ponemos en 0 el numero de interrupciones porque los delays ocacionan que al volver al flujo normal de programa el conteo de los interrup siguio aumentando y eso hace que el motor se apage, por ejemplo al avisar de alarma de temperatura, entramos al audio alarma donde le toma 1.2 segundos terminar el audio durante ese tiempo las interrupt si siguieron aumentando, si el conteo es de 16 interrup cada 250ms es decir 1000rpm, al finalisar el audio son 80 interrup porque nunca entra al codigo que reinicia el conteo de interrupt, esto le dice al sistema que el motor va a 5000rpm y apaga el motor. 

}

void Audio::alarma()
{
    for (int i = 0; i < 5; i++) {
        ExternalWDT::reset();
        digitalWrite(PIN_BUZZER, HIGH);
        delay(200);
        digitalWrite(PIN_BUZZER, LOW);
        delay(30);
    }
    //numInterrupt = 0;
}

void Audio::ligero()
{
    for (int i = 0; i < 3; i++) {
        ExternalWDT::reset();
        digitalWrite(PIN_BUZZER, HIGH);
        delay(100);
        digitalWrite(PIN_BUZZER, LOW);
        delay(30);
    }
    //numInterrupt = 0;
}

void Audio::medio()
{
    for (int i = 0; i < 4; i++) {
        ExternalWDT::reset();
        digitalWrite(PIN_BUZZER, HIGH);
        delay(150);
        digitalWrite(PIN_BUZZER, LOW);
        delay(30);
    }
    //numInterrupt = 0;
}

void Audio::grave()
{
    for (int i = 0; i < 3; i++) {
        ExternalWDT::reset();
        digitalWrite(PIN_BUZZER, HIGH);
        delay(400);
        digitalWrite(PIN_BUZZER, LOW);
        delay(30);
    }
    //numInterrupt = 0;
}

void Audio::exito()
{
    digitalWrite(PIN_BUZZER, HIGH);
    delay(300);
    digitalWrite(PIN_BUZZER, LOW);
    delay(30);
    for (int i = 0; i < 2; i++) {
        ExternalWDT::reset();
        digitalWrite(PIN_BUZZER, HIGH);
        delay(80);
        digitalWrite(PIN_BUZZER, LOW);
        delay(30);
    }
    digitalWrite(PIN_BUZZER, HIGH);
    delay(300);
    digitalWrite(PIN_BUZZER, LOW);
    delay(30);
}

void Audio::guardar()
{
    for (int i = 0; i < 6; i++) {
        ExternalWDT::reset();
        digitalWrite(PIN_BUZZER, HIGH); //si se escribio algo en la memoria activa el PIN_BUZZER hace un tono con el PIN_BUZZER
        delay(50);
        digitalWrite(PIN_BUZZER, LOW);
        delay(30);
    }
    delay(800);
}
