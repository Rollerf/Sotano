#include "Portal.h"
#include "Arduino.h"
#include <Timer.h>
#include <Switches.h>

#define DEBOUNCE_TIME 7

Switches *SwitchLsOpened;
Switches *SwitchLsClosed;

Portal::Portal(unsigned char pinOutput, unsigned char pinLsOpened, unsigned char pinLsClosedInput)
{
    pinMode(pinOutput, OUTPUT);
    pinMode(pinLsOpened, INPUT_PULLUP);
    pinMode(pinLsClosedInput, INPUT_PULLUP);

    digitalWrite(pinOutput, HIGH);

    SwitchLsOpened = new Switches(DEBOUNCE_TIME, pinLsOpened);
    SwitchLsClosed = new Switches(DEBOUNCE_TIME, pinLsClosedInput);

    this->pinOutput = pinOutput;
}

void Portal::press()
{
    digitalWrite(pinOutput, LOW);
}

void Portal::release()
{
    digitalWrite(pinOutput, HIGH);
}

bool Portal::getLsOpened()
{
    return SwitchLsOpened->switchMode(false);
}

bool Portal::getLsClosed()
{
    return SwitchLsClosed->switchMode(false);
}
