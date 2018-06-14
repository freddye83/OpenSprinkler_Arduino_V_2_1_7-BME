/* ==========================================================================================================
This is a fork of Rays OpenSprinkler code thats amended to use alternative hardware:

EtherCardW5100.h and EtherCardW5100.cpp implements a minimal set of functions
as a wrapper to replace the ENC28J60 EtherCard class libraries with the standard
Arduino Wiznet5100 Ethernet library.

Version:     Opensprinkler 2.1.8

Date:        May 2018

Repository:  https://github.com/plainolddave/OpenSprinkler-Arduino

License:     Creative Commons Attribution-ShareAlike 3.0 license

Refer to the README file for more information

========================================================================================================== */

#ifndef _OPENSPRINKLER_ARDUINO_BUTTON_h
#define _OPENSPRINKLER_ARDUINO_BUTTON_h

#include <Arduino.h>
#include "defines.h"

class OpenSprinkler_Arduino_ButtonClass
{
protected:
    byte sample();
    byte readBusy(byte pinButton, byte waitMode, byte button, byte isHolding);

 public:
	void begin();
    byte read(byte waitMode);
    void print();
};

extern OpenSprinkler_Arduino_ButtonClass OpenSprinklerButton;

#endif

