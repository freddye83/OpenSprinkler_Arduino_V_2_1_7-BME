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

#include "OpenSprinkler_Arduino_Station.h"

const uint8_t OpenSprinkler_Arduino_StationClass::maxBoards = 3;
const uint8_t OpenSprinkler_Arduino_StationClass::pins[24] =
{
    PIN_STN_S01, PIN_STN_S02, PIN_STN_S03, PIN_STN_S04, PIN_STN_S05, PIN_STN_S06, PIN_STN_S07, PIN_STN_S08,
    PIN_STN_S09, PIN_STN_S10, PIN_STN_S11, PIN_STN_S12, PIN_STN_S13, PIN_STN_S14, PIN_STN_S15, PIN_STN_S16,
    PIN_STN_S17, PIN_STN_S18, PIN_STN_S19, PIN_STN_S20, PIN_STN_S21, PIN_STN_S22, PIN_STN_S23, PIN_STN_S24
};

void OpenSprinkler_Arduino_StationClass::begin()
{
    // initialize the Digital IO pins as outputs:
    for (int i = 0; i < (MAX_EXT_BOARDS * 8); i++)
    {
        pinMode(pins[i], OUTPUT);
        digitalWrite(pins[i], PIN_STATE_OFF);
    }
}

void OpenSprinkler_Arduino_StationClass::apply(byte enabled, byte *stationBytes, byte maxExtensionBoards)
{
    byte boards;

    // double check the number of boards, just in case (n.b. includes the onboard 8 stations)
    boards = constrain(maxExtensionBoards + 1, 0, maxBoards);

    // Shift out all station bit values from the highest bit to the lowest
    for (byte boardID = 0; boardID < boards; boardID++)
    {
        if (enabled)
            set(boardID, stationBytes[boardID]);
        else
            set(boardID, 0);
    }
}

void OpenSprinkler_Arduino_StationClass::set(byte boardID, byte stationByte)
{
    for (uint8_t stationID = 0; stationID < 8; stationID++)
    {
        digitalWrite(pins[(boardID * 8) + stationID], (stationByte & ((byte)1 << stationID)) ? PIN_STATE_OFF : PIN_STATE_ON);
    }
}

void OpenSprinkler_Arduino_StationClass::print(byte enabled, byte *stationBytes)
{
    uint8_t pinstate = 0;

    if (hour() <= 9)
        DEBUG_PRINT(F("0"));
    DEBUG_PRINT(hour());
    DEBUG_PRINT(F(":"));

    if (minute() <= 9)
        DEBUG_PRINT(F("0"));
    DEBUG_PRINT(minute());
    DEBUG_PRINT(F(":"));

    if (second() <= 9)
        DEBUG_PRINT(F("0"));
    DEBUG_PRINT(second());
    DEBUG_PRINT(F(" Stations: "));

    // Print out all station pin values
    for (byte boardId = 0; boardId < maxBoards; boardId++)
    {
        for (uint8_t stationID = 0; stationID < 8; stationID++)
        {
            if (digitalRead(boardId * 8 + stationID) == PIN_STATE_OFF)
            {
                DEBUG_PRINT(stationID);
            }
            else
            {
                DEBUG_PRINT(F("."));
            }
        }
        DEBUG_PRINT(F(" "));
    }
    DEBUG_PRINTLN();
}

OpenSprinkler_Arduino_StationClass OpenSprinklerStation;

