#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

// No I2C on this board
static const uint8_t SDA = -1;
static const uint8_t SCL = -1;

// Default SPI mapped to Radio
static const uint8_t MISO = 3;
static const uint8_t SCK = 5;
static const uint8_t MOSI = 6;
static const uint8_t SS = 7;

#endif /* Pins_Arduino_h */
