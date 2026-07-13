#ifndef _VARIANT_TINYLORA_MV_GAME_
#define _VARIANT_TINYLORA_MV_GAME_

// =============================================================================
// TinyLora_MV_GAME
// ESP32-S3 N16R8 (16MB flash, 8MB PSRAM), USB CDC for serial debug
// Reference: sakurapi-mv (slimmed down)
// =============================================================================

// -----------------------------------------------------------------------------
// Display (SSD1306 over I2C, 128x64 OLED)
// -----------------------------------------------------------------------------
#define USE_SSD1306

#define I2C_SDA 18
#define I2C_SCL 17

#define WIRE_INTERFACES_COUNT 1

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------
#define LED_PIN 48
#define LED_STATE_ON 1

// -----------------------------------------------------------------------------
// Button (BOOT button on GPIO0, active LOW with internal pull-up)
// -----------------------------------------------------------------------------
#define BUTTON_PIN 0
#define BUTTON_NEED_PULLUP

// -----------------------------------------------------------------------------
// Battery (voltage divider on GPIO1, ADC1 channel 1)
// -----------------------------------------------------------------------------
#define BATTERY_PIN 1
#define ADC_CHANNEL ADC1_GPIO1_CHANNEL
#define ADC_MULTIPLIER 2.0f

// -----------------------------------------------------------------------------
// LoRa (SX1262 module)
// -----------------------------------------------------------------------------
#define USE_SX1262

// SPI bus — uses VSPI (default SPI instance on ESP32-S3)
#define LORA_SCK  5
#define LORA_MISO 3
#define LORA_MOSI 6
#define LORA_CS   7

#define LORA_DIO0 RADIOLIB_NC
#define LORA_DIO1 16
#define LORA_DIO2 RADIOLIB_NC // Busy line is on LORA_BUSY below
#define LORA_BUSY  15
#define LORA_RESET 8

// Aliased names for SX126x driver
#define SX126X_CS         LORA_CS
#define SX126X_DIO1       LORA_DIO1
#define SX126X_BUSY       LORA_BUSY
#define SX126X_RESET      LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define TCXO_OPTIONAL

// -----------------------------------------------------------------------------
// GPS — temporarily disabled. Code is staged below; uncomment when adding
// a GPS module to this board.
// -----------------------------------------------------------------------------
// #define HAS_GPS 1
//
// #undef GPS_RX_PIN
// #undef GPS_TX_PIN
// #define GPS_RX_PIN    44
// #define GPS_TX_PIN    43
// #define PIN_GPS_EN    38
// #define GPS_EN_ACTIVE 1

#endif /* _VARIANT_TINYLORA_MV_GAME_ */
