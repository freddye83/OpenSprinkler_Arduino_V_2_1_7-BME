/* ====================================================

Version:	OpenSprinkler_Arduino 2.1.7

Date:		May 2018

Repository: https://github.com/plainolddave/OpenSprinkler-Arduino

License:	Creative Commons Attribution-ShareAlike 3.0 license

About:		This is a fork of Rays OpenSprinkler code thats amended to use alternative hardware:
                - Arduino Mega 2560 (or any Arduino MCU that can handle compiled code size of around 70K)
                - Freetronics LCD Keypad Shield
                - Discrete IO outputs instead of using a shift register
                - Either:
                    - Enc28j60 Ethernet with external SD Card or
                    - Wiznet W5100 Ethernet (i.e. 'standard' for Arduino)

            In general the approach is to make minimum changes necessary to:
                1) deconflict between Rays libraries that have been modified away from standard Arduino
                2) get alternative hardware to run
                3) debug

            PLUS this version adds a couple of additional functions:
                - reboot daily to ensure stable operation
                - option to display free memory on the LCD for debugging
                - heartbeat function to say 'alls well' - flashes an LED and the ':' on the LCD time at 1Hz
                - option to turn the WDT on or off (refer to your reference documentation as to whether WDT is supported by the bootloader on your arduino)

Otherwise the code is largely 'as is' from https://github.com/OpenSprinkler/OpenSprinkler-Firmware

Changes from Rays original code are marked with OPENSPRINKLER_ARDUINO (or variations thereof)

As always - FULL CREDIT to Ray for all his hard work to build and maintain the Open Sprinkler project!
*/

/* OpenSprinkler Unified (AVR/RPI/BBB/LINUX) Firmware
* Copyright (C) 2015 by Ray Wang (ray@opensprinkler.com)
*
* Main loop wrapper for Arduino
* Feb 2015 @ OpenSprinkler.com
*
* This file is part of the OpenSprinkler Firmware
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see
* <http://www.gnu.org/licenses/>.
*/

// This is the main configuration file to set up Opensprinkler Arduino
#include <Arduino.h>
#include <Wire.h>
#ifndef ESP8266
    #include <SdFat.h>
#endif

#include "OpenSprinkler.h"

extern OpenSprinkler os;

#ifdef OPENSPRINKLER_ARDUINO_W5100  

    EtherCardWrapper<EtherCardW5100> ether;
    byte EtherCardW5100::buffer[ETHER_BUFFER_SIZE];

#elif defined (OPENSPRINKLER_ARDUINO_ENC28J60) // OPENSPRINKLER_ARDUINO_W5100

    EtherCardWrapper<EtherCard> ether;
    byte Ethernet28J60::buffer[ETHER_BUFFER_SIZE];

#else   // OPENSPRINKLER_ARDUINO_ENC28J60
    #error Please define a network card in OpenSprinkler_Arduino.h
#endif  // OPENSPRINKLER_ARDUINO

void do_setup();
void do_loop();

void setup()
{
#ifdef OPENSPRINKLER_ARDUINO_AUTOREBOOT   
    Alarm.alarmRepeat(REBOOT_HR, REBOOT_MIN, REBOOT_SEC, reboot);
#endif 

    do_setup();
}

void loop()
{
    do_loop();
}

// Added for Auto Reboot
#ifdef OPENSPRINKLER_ARDUINO_AUTOREBOOT 
void(*resetPointer) (void) = 0;			// AVR software reset function
void reboot()
{
    resetPointer();
}
#endif // OPENSPRINKLER_ARDUINO_AUTOREBOOT
