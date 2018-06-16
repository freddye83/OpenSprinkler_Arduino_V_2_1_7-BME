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
				- ability to reboot daily to ensure stable operation
				- ability to display free memory on the LCD for debugging
				- heartbeat function to say 'alls well' - flashes an LED and the ':' on the LCD time at 1Hz
				- ability to turns the WDT on or off (refer to your reference documentationas to whether WDT is supported by the bootloader on your arduino)

			Otherwise the code is 'as is' from https://github.com/OpenSprinkler/OpenSprinkler-Firmware

			Changes from Rays original code are marked with OPENSPRINKLER_ARDUINO (or variations thereof)

			As always - FULL CREDIT to Ray for all his hard work to build and maintain the Open Sprinkler project!
*/
#ifndef _OS_ARDUINO_H_
#define _OS_ARDUINO_H_

// =================================================================
// Compiler switches - comment out these defines to substitute different hardware and torun on/off functionality
// You can also search the project for these keywords to find where the code has been modified
// =================================================================

#define OPENSPRINKLER_ARDUINO
#define OPENSPRINKLER_ARDUINO_DISCRETE			// use discrete IO instead of a shift register to control sprinkler outputs
//#define OPENSPRINKLER_ARDUINO_DISCRETE_INVERT	// uncomment this line to invert the logic of the discrete IO (LOW to HIGH and vice versa) 
#define OPENSPRINKLER_ARDUINO_FREETRONICS_LCD	// use Freetronics LCD with keypad
#define OPENSPRINKLER_ARDUINO_W5100			// use the standard Wiznet5100 Ethernet interface
//#define OPENSPRINKLER_ARDUINO_ENC28J60		    // use the less common (but cheaper) enc28j60 Ethernet interface, as per original opensprinkler hardware
#define OPENSPRINKLER_ARDUINO_AUTOREBOOT		// this is an optional function to reboot daily to ensure stable operation
//#define OPENSPRINKLER_ARDUINO_FREEMEM			// this is an optional function to display free memory on the LCD for debugging
#define OPENSPRINKLER_ARDUINO_HEARTBEAT			// this is an optional function to say 'alls well' - flashes an LED, and the ':' on the LCD time at 1Hz
//#define OPENSPRINKLER_ARDUINO_WDT				// this flag turns the WDT on or off (refer to your reference documentation 
												// for whether the firmware loaded on your Arduino mega supports a WDT)
#define OPENSPRINKLER_BME280 // Utilisation d'un capteur BME280 I2C
// =================================================================
#ifdef OPENSPRINKLER_ARDUINO

    #include <Arduino.h>
    #include "src/Time.h"
    #include "src/DS1307RTC.h"
    #include "Opensprinkler_Arduino_Undef.h"

    /* READ ME - PIN_EXT_BOARDS defines the total number of discrete digital IO pins
    used to control watering stations. There are 8 stations per extender board, that
    MUST have PIN_EXT_BOARDS x 8  pins defined below (otherwise you'll get out of range
    issues with the array of pins) 
    */
    #define MAX_EXT_BOARDS		2                       // maximum number of exp. boards (each expands 8 stations, plus the onboard 8 stations)
    #define MAX_NUM_STATIONS    ((1+MAX_EXT_BOARDS)*8)  // maximum number of stations (including the onboard 8 stations)
    #define NVM_SIZE            2048                    // For AVR, nvm data is stored in EEPROM, ATmega644 has 2K EEPROM

    /*
    PINOUT AND DEFINES FOR OPENSPRINKLER_ARDUINO

    The following are the pins used by Dave's Arduino opensprinkler
    This assumes an Arduino ATMEGA2560 - you'll need to adjust the
    pins accordingly for your hardware config

    The pinouts below are arranged by pin number to ensure no conflicts
    */
    #define PIN_D0				0		// Serial port - do not use
    #define PIN_D1				1		// Serial port - do not use
    #define PIN_LCD_D4			2		// Freetronics LCD - see Note 1 below
    #define PIN_LCD_BACKLIGHT	3		// Freetronics LCD backlight
    #define PIN_W5100_SD_CS		4		// W5100 SD card chip select - see Note 1 below
    #define PIN_LCD_D5			5		// Freetronics LCD
    #define PIN_LCD_D6			6		// Freetronics LCD
    #define PIN_LCD_D7			7		// Freetronics LCD

    #define PIN_LCD_RS			8		// Freetronics LCD
    #define PIN_LCD_EN			9		// Freetronics LCD
    #define PIN_W5100_ETHER_CS	10		// W5100 Ethernet chip select pin
    #define PIN_BUTTON_1		11      // Button 1 - may not be necessary (Note 4 below)
    #define PIN_BUTTON_2	    12      // Button 2 - may not be necessary (Note 4 below)
    #define PIN_HEARTBEAT		13		// Pin to show a heartbeat LED 

    #define PIN_D14				14		// Serial 3 (don't use unless you really want to)
    #define PIN_D15				15		// Serial 3 (don't use unless you really want to)
    #define PIN_D16				16		// Serial 2 (don't use unless you really want to)
    #define PIN_D17				17		// Serial 2 (don't use unless you really want to)
    #define PIN_D18				18		// Serial 1 (don't use unless you really want to)
    #define PIN_D19				19		// Serial 1 (don't use unless you really want to)
    #define PIN_D20				20		// I2C Interface - SDA (do not use) 
    #define PIN_D21				21		// I2C Interface - SCL (do not use)

    #define PIN_STN_S09			22		// Watering stations 17 to 24 (Note 2 below)
    #define PIN_STN_S13			23		// Watering stations 17 to 24 (Note 2 below)
    #define PIN_STN_S10			24		// Watering stations 17 to 24 (Note 2 below)
    #define PIN_STN_S14			25		// Watering stations 17 to 24 (Note 2 below)
    #define PIN_STN_S11			26		// Watering stations 17 to 24 (Note 2 below)
    #define PIN_STN_S15			27		// Watering stations 17 to 24 (Note 2 below)
    #define PIN_STN_S12			28		// Watering stations 17 to 24 (Note 2 below)
    #define PIN_STN_S16			29		// Watering stations 17 to 24 (Note 2 below)

    #define PIN_STN_S17			30		// Watering stations 9 to 16 (Note 2 below)
    #define PIN_STN_S18			31		// Watering stations 9 to 16 (Note 2 below)
    #define PIN_STN_S19			32		// Watering stations 9 to 16 (Note 2 below)
    #define PIN_STN_S20			33		// Watering stations 9 to 16 (Note 2 below)
    #define PIN_ENC28J60_SD_CS	34		// SD card chip select pin for Dave's ENC28J60 config
    #define PIN_BUTTON_3		35		// Button 3 - may not be necessary (Note 4 below)
    #define PIN_STN_S21			36		// Watering stations 9 to 16 (Note 2 below)
    #define PIN_STN_S22			37		// Watering stations 9 to 16 (Note 2 below)
    #define PIN_STN_S23			38		// Watering stations 9 to 16 (Note 2 below)
    #define PIN_STN_S24			39		// Watering stations 9 to 16 (Note 2 below)

    #define PIN_STN_S04			40		// Watering stations 1 to 8 (Note 2 below)	
    #define PIN_STN_S08			41		// Watering stations 1 to 8 (Note 2 below)
    #define PIN_STN_S03			42		// Watering stations 1 to 8 (Note 2 below)
    #define PIN_STN_S07			43		// Watering stations 1 to 8 (Note 2 below)
    #define PIN_STN_S02			44		// Watering stations 1 to 8 (Note 2 below)
    #define PIN_STN_S06			45		// Watering stations 1 to 8 (Note 2 below)
    #define PIN_STN_S01			46		// Watering stations 1 to 8 (Note 2 below)
    #define PIN_STN_S05			47		// Watering stations 1 to 8 (Note 2 below)

    #define PIN_D48				48		// currently assigned to PIN_RFTX - see below
    #define PIN_D49				49		// not used (Note 4 below)
    #define PIN_D50				50		// SPI Interface (don't use unless you want conflicts)
    #define PIN_D51				51		// SPI Interface (don't use unless you want conflicts)
    #define PIN_D52				52		// SPI Interface (don't use unless you want conflicts)
    #define PIN_ENC28J60_CS	    53		// Ethernet controller chip select pin for Daves' ENC28J60 config

    #define BUTTON_ADC_PIN		A0		// A0 is the LCD button ADC input
    #define PIN_ANALOG1		    A1		// not used (Note 4 below)
    #define PIN_ANALOG2		    A2		// not used (Note 4 below)
    #define PIN_ANALOG3		    A3		// currently assigned to PINX_RF - see below
    #define PIN_ANALOG4		    A4		// not used (Note 4 below)
    #define PIN_ANALOG5		    A5		// not used (Note 4 below)
    #define PIN_ANALOG6		    A6		// not used (Note 4 below)
    #define PIN_ANALOG7		    A7		// not used (Note 4 below)

    // The pins below are unused - adjust them for your hardware 
    #define PIN_LCD_CONTRAST	A8		// LCD contrast pin (Note 3 below / not used)  
    #define PIN_RF_DATA			A9		// RF data pin (Note 3 below / not used)
    #define PIN_RAINSENSOR		A10		// rain sensor (Note 3 below / not used)
    #define PIN_FLOWSENSOR		A11		// flow sensor (currently shared with rain sensor) (Note 3 below / not used)
    #define PIN_FLOWSENSOR_INT	A12		// flow sensor interrupt pin (INT1)  (Note 3 below / not used)
    #define PIN_EXP_SENSE		A13		// expansion board sensing pin (A4) (Note 3 below / not used)
    #define PIN_CURR_SENSE		A14		// current sensing pin (A7) (Note 3 below / not used)
    #define PIN_CURR_DIGITAL	A15		// digital pin index for A7 (Note 3 below / not used)

    /* 
    Notes:

    Note 1 -	D4 on the Freetronics LCD shield clashes with the chipselect pin for the SD card that is also D4 on some W5100 shields.
    You may need to jumper it to D2 as described at: http://forum.freetronics.com/viewtopic.php?t=770

    Note 2 -	These pins provide the control signal to switch watering solenoids on and off
    (i.e. not using a shift register like the regular opensprinkler hardware). If your processor has more or less pins,
    then define watering stations in groups of 8 pins. Make sure to also set MAX_EXT_BOARDS abiove. 

    Note 3 -	This define is not used by opensprinkler_arduino hardware - it is just set to an unused pin / should be harmless
    You'll need to set the correct pins for these items if you have them connected

    Note 4 -	You can use these pins for an extra expansion board or GPIO if you need (must be in increments of x 8)
    */

    // Define the chipselect pins for all SPI devices attached to the arduino
    // Unused pins needs to be pulled high otherwise SPI doesn't work properly
    #define PIN_SPI_DEVICES   4 		    // number of SPI devices
    const uint8_t PIN_SPI_CS_LIST[] =	    // CS pin for each device
    {
        PIN_W5100_SD_CS,
        PIN_W5100_ETHER_CS,
        PIN_ENC28J60_SD_CS,
        PIN_ENC28J60_CS
    };

    const uint8_t PIN_FREE_LIST[] =         // Free GPIO pins
    {
        PIN_BUTTON_1,
        PIN_BUTTON_2,
        PIN_BUTTON_3,
        PIN_D14,
        PIN_D15,
        PIN_D16,
        PIN_D17,
        PIN_D18,
        PIN_D19,
        PIN_D49,
        PIN_ANALOG1,
        PIN_ANALOG2,
        PIN_ANALOG4,
        PIN_ANALOG5,
        PIN_ANALOG6,
        PIN_ANALOG7
    };

    #define PIN_RFTX                PIN_D48     // RF data pin (default for opensprinkler is pin 28)
    #define PORT_RF				    PORTA
    #define PINX_RF				    PIN_ANALOG3 // RF data pin (default for opensprinkler is PINA3)

    // These are not used - map them to some pin
    #define PIN_SR_LATCH    A8
    #define PIN_SR_DATA     A8
    #define PIN_SR_CLOCK    A8
    #define PIN_SR_OE       A8

#endif // OPENSPRINKLER_ARDUINO

// =================================================================
#ifdef OPENSPRINKLER_ARDUINO_DISCRETE
    
    #include "OpenSprinkler_Arduino_Station.h"

    #ifdef OPENSPRINKLER_ARDUINO_DISCRETE_INVERT
        #define PIN_STATE_ON    LOW
        #define PIN_STATE_OFF   HIGH
    #else
        #define PIN_STATE_ON    HIGH
        #define PIN_STATE_OFF   LOW
    #endif // OPENSPRINKLER_ARDUINO_DISCRETE_INVERT

#endif

// =================================================================
#ifdef OPENSPRINKLER_ARDUINO_FREETRONICS_LCD

    // some example macros with friendly labels for LCD backlight / pin control
    #define LCD_BACKLIGHT_OFF()			digitalWrite( PIN_LCD_BACKLIGHT, LOW )
    #define LCD_BACKLIGHT_ON()			digitalWrite( PIN_LCD_BACKLIGHT, HIGH )
    #define LCD_BACKLIGHT_STATE(state)	{if( state ){digitalWrite( PIN_LCD_BACKLIGHT, HIGH );}else{digitalWrite( PIN_LCD_BACKLIGHT, LOW );} }

    // ADC readings expected for the 5 buttons on the ADC input
    #define RIGHT_10BIT_ADC		0		// right
    #define UP_10BIT_ADC		  99 //145		// up
    #define DOWN_10BIT_ADC		256 // 329		// down
    #define LEFT_10BIT_ADC		410 //505		// left
    #define SELECT_10BIT_ADC	640 // 741		// select
    #define BUTTONHYSTERESIS	10		// hysteresis for valid button sensing window

    #define BUTTON_RIGHT		1		// values used for detecting analog buttons
    #define BUTTON_UP			2		// 
    #define BUTTON_DOWN			3		// 
    #define BUTTON_LEFT			4		// 
    #define BUTTON_SELECT		5		//  

    #include "src/LiquidCrystal.h"
    #include "OpenSprinkler_Arduino_Button.h"

#else
    #include "src/LiquidCrystal.h"
#endif // OPENSPRINKLER_ARDUINO_FREETRONICS_LCD

// =================================================================
#ifdef OPENSPRINKLER_ARDUINO_W5100

    #include "src/ethercard/EtherCardWrapper.h"
    #include "src/ethercard/EtherCardW5100.h"

    // Set up the right pins for the ethernet controller in use
    #define PIN_ETHER_CS    PIN_W5100_ETHER_CS
    #define PIN_SD_CS       PIN_W5100_SD_CS

    extern EtherCardWrapper<EtherCardW5100> ether; // Global declaration of ether

#elif defined (OPENSPRINKLER_ARDUINO_ENC28J60)

    #include "src/ethercard/EtherCardWrapper.h"
    #include "src/ethercard/EtherCard.h"

    // Set up the right pins for the ethernet controller in use
    #define PIN_ETHER_CS    PIN_ENC28J60_CS
    #define PIN_SD_CS       PIN_ENC28J60_SD_CS

    extern EtherCardWrapper<EtherCard> ether; // Global declaration of ether

#else
    #error Please select a network interface
#endif

// =================================================================

#ifdef OPENSPRINKLER_ARDUINO_AUTOREBOOT

    #include "src/TimeAlarms.h"

    // Auto reboot time
    #define REBOOT_HR				12		// hour to perform daily reboot
    #define REBOOT_MIN				00		// min  to perform daily reboot
    #define REBOOT_SEC				00		// sec  to perform daily reboot

#endif // OPENSPRINKLER_ARDUINO_AUTOREBOOT

// =================================================================

#ifdef OPENSPRINKLER_ARDUINO_FREEMEM

    #include "src/MemoryFree.h"

#endif // OPENSPRINKLER_ARDUINO_FREEMEM

// =================================================================

#ifdef OPENSPRINKLER_ARDUINO_HEARTBEAT

#endif // OPENSPRINKLER_ARDUINO_HEARTBEAT

// =================================================================

#ifdef OPENSPRINKLER_ARDUINO_WDT

    #define 	wdt_reset()   __asm__ __volatile__ ("wdr")  // watchdog timer reset

#else

    // Only needed if not using WDT (to ensure the WDT is disbled)
    #include <avr/wdt.h>

#endif // OPENSPRINKLER_ARDUINO_WDT

// =================================================================

#ifdef SERIAL_DEBUG
#define DEBUG_DELAY(x) delay(x)
#define DEBUG_PRINTF(x,y) Serial.print(x, y)
#else
#define DEBUG_DELAY(x) {}
#define DEBUG_PRINTF(x,y) {}
#endif

//typedef const char prog_char;
//typedef const char prog_uchar;

#endif  // _OS_ARDUINO_H_




