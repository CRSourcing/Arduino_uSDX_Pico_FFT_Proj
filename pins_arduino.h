
#pragma once
#include <macros.h>
#include <stdint.h>

#ifndef __PINS_ARDUINO__
#define __PINS_ARDUINO__

#ifdef __cplusplus
extern "C" unsigned int PINCOUNT_fn();
#endif

// Pin count
// ----
#define PINS_COUNT           (PINCOUNT_fn())
#define NUM_DIGITAL_PINS     (30u)
#define NUM_ANALOG_INPUTS    (4u)
#define NUM_ANALOG_OUTPUTS   (0u)

extern PinName digitalPinToPinName(pin_size_t P);

// LEDs
// ----
#define PIN_LED     (25u)
#define LED_BUILTIN PIN_LED

// Analog pins
// -----------
#define PIN_A0 (26u)
#define PIN_A1 (27u)
#define PIN_A2 (28u)
#define PIN_A3 (29u)

static const uint8_t A0  = PIN_A0;
static const uint8_t A1  = PIN_A1;
static const uint8_t A2  = PIN_A2;
static const uint8_t A3  = PIN_A3;

#define ADC_RESOLUTION 12

// Serial
#define PIN_SERIAL_TX (0ul)
#define PIN_SERIAL_RX (1ul)

// SPI
#define PIN_SPI_MISO  (12u)
#define PIN_SPI_MOSI  (11u)
#define PIN_SPI_SCK   (10u)
#define PIN_SPI_SS    (17u)

static const uint8_t SS   = PIN_SPI_SS;   // SPI Slave SS not used. Set here only for reference.
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

// Wire


#define WIRE_HOWMANY    (2)  //included I2C1   default was 1
#define I2C_SDA       (digitalPinToPinName(PIN_WIRE_SDA))   //I2C0
#define I2C_SCL       (digitalPinToPinName(PIN_WIRE_SCL))   //I2C0
#define I2C_SDA1      (digitalPinToPinName(PIN_WIRE_SDA1))  //included I2C1
#define I2C_SCL1      (digitalPinToPinName(PIN_WIRE_SCL1))  //included I2C1
#define PIN_WIRE_SDA        (16u)  //I2C0
#define PIN_WIRE_SCL        (17u)  //I2C0
#define PIN_WIRE_SDA1       (18u)  //included I2C1
#define PIN_WIRE_SCL1       (19u)  //included I2C1

#define SERIAL_HOWMANY		1
#define SERIAL1_TX			(digitalPinToPinName(PIN_SERIAL_TX))
#define SERIAL1_RX			(digitalPinToPinName(PIN_SERIAL_RX))

#define SERIAL_CDC			1
#define HAS_UNIQUE_ISERIAL_DESCRIPTOR
#define BOARD_VENDORID		0x2e8a
#define BOARD_PRODUCTID		0x00c0
#define BOARD_NAME			"RaspberryPi Pico"

uint8_t getUniqueSerialNumber(uint8_t* name);
void _ontouch1200bps_();

#define SPI_HOWMANY		(1)
#define SPI_MISO		(digitalPinToPinName(PIN_SPI_MISO))
#define SPI_MOSI		(digitalPinToPinName(PIN_SPI_MOSI))
#define SPI_SCK			(digitalPinToPinName(PIN_SPI_SCK))

#define WIRE_HOWMANY	(2)	// modified

// Wire
#define I2C_SDA			(digitalPinToPinName(PIN_WIRE_SDA))	 	// modified
#define I2C_SCL			(digitalPinToPinName(PIN_WIRE_SCL))		// modified

// Wire1
#define I2C_SDA1		(digitalPinToPinName(PIN_WIRE_SDA1))	// modified
#define I2C_SCL1		(digitalPinToPinName(PIN_WIRE_SCL1))	// modified

#define digitalPinToPort(P)		(digitalPinToPinName(P)/32)

#define SERIAL_PORT_USBVIRTUAL      SerialUSB
#define SERIAL_PORT_MONITOR         SerialUSB
#define SERIAL_PORT_HARDWARE        Serial1
#define SERIAL_PORT_HARDWARE_OPEN   Serial1

#define USB_MAX_POWER	(500)

#endif //__PINS_ARDUINO__
