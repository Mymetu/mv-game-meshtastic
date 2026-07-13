#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 41;
static const uint8_t RX = 42;

static const uint8_t SDA = 35;
static const uint8_t SCL = 36;

// Default SPI will be mapped to Radio
static const uint8_t MISO = 37;
static const uint8_t SCK = 40;
static const uint8_t MOSI = 38;
static const uint8_t SS = 39;

#endif /* Pins_Arduino_h */