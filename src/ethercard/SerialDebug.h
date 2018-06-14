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
#pragma once

#ifndef _OPENSPRINKLER_ARDUINO_DEBUG_H_
#define _OPENSPRINKLER_ARDUINO_DEBUG_H_

#ifndef SERIAL_DEBUG
#define SERIAL_DEBUG
#endif

#ifdef SERIAL_DEBUG
#define DEBUG_BEGIN(x)   Serial.begin(x)
#define DEBUG_PRINT(x)   Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTIP(x) ether.printIp("IP:",x)
#define DEBUG_DELAY(x) delay(x)
#define DEBUG_PRINTF(x,y) Serial.print(x, y)
#else
#define DEBUG_BEGIN(x)   {}
#define DEBUG_PRINT(x)   {}
#define DEBUG_PRINTLN(x) {}
#define DEBUG_PRINTIP(x) {}
#define DEBUG_DELAY(x) {}
#define DEBUG_PRINTF(x,y) {}
#endif

#endif  // _OPENSPRINKLER_ARDUINO_DEBUG_H_





