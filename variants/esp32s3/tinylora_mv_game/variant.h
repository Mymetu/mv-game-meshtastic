#ifndef _VARIANT_TINYLORA_MV_GAME_
#define _VARIANT_TINYLORA_MV_GAME_

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/

// General status LED
// #define LED_PIN (2)  // On-board LED pin

// Button
// #define BUTTON_PIN (0)  // BOOT button
// #define BUTTON_NEED_PULLUP

#define I2C_SDA 35
#define I2C_SCL 16

#define BUTTON_PIN (0)
#define BUTTON_ACTIVE_LOW true
#define BUTTON_ACTIVE_PULLUP true
/*----------------------------------------------------------------------------
 *        Display (ST7789 135x240)
 *----------------------------------------------------------------------------*/

// #define USE_ST7789
#define ST7789_SPI_HOST SPI3_HOST
#define DISPLAY_240_135

#undef OLED_CJK_SIZE
#define OLED_CJK_SIZE 16
#define EMOTE_FULL
// #define HAS_TFT 1

#define TFT_MESH_OVERRIDE COLOR565(234,225,231)  //NIPPON VC0042-4

// Screen dimensions
#define USE_TFTDISPLAY 1
#define TFT_COLOR_SUPPORT 1
#ifdef DISPLAY_240_135
#define TFT_MEMORY_WIDTH 240
#define TFT_WIDTH 240
#define TFT_MEMORY_HEIGHT 240
#define TFT_HEIGHT 240
#define TFT_OFFSET_X -53
#define TFT_OFFSET_Y 40
#define TFT_OFFSET_ROTATION 2
#else
#define TFT_WIDTH 240
#define TFT_HEIGHT 240
#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y -80
#define TFT_OFFSET_ROTATION 0
#endif

// SPI pins for display (shared with LoRa)
#define ST7789_CS ST7789_NSS
#define ST7789_SCK 16
#define ST7789_SDA 15
#define ST7789_MISO -1
#define ST7789_BUSY -1
#define ST7789_NSS 5
#define ST7789_DC 6
#define ST7789_RS ST7789_DC
#define ST7789_RESET 7
#define ST7789_BL 4
#define TFT_BL  ST7789_BL
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
#define SCREEN_TRANSITION_FRAMERATE 3 // fps
#define VTFT_LEDA 4   // Backlight
#define VTFT_CTRL -1  // Control pin (not used)

/*----------------------------------------------------------------------------
 *        LoRa (SX1262 with TCXO)
 *----------------------------------------------------------------------------*/

#define USE_SX1262

// SPI pins
#define LORA_SCK 40
#define LORA_MISO 38
#define LORA_MOSI 21
#define LORA_CS 39

#define LORA_DIO0 47
#define LORA_DIO1 RADIOLIB_NC
#define LORA_DIO2 RADIOLIB_NC
#define LORA_BUSY 14                    // BUSY pin for SX1262
#define LORA_RESET 1

#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO0           // SX1278 use DIO0, SX1262 use DIO1 as IRQ
#define SX126X_BUSY LORA_BUSY
#define SX126X_RESET LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH

#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define TCXO_OPTIONAL

/*----------------------------------------------------------------------------
 *        GPS
 *----------------------------------------------------------------------------*/

// #define HAS_GPS 1
// #define GPS_RX_PIN 42
// #define GPS_TX_PIN 41
// #define GPS_BAUDRATE 9600

/*----------------------------------------------------------------------------
 *        Battery
 *----------------------------------------------------------------------------*/

// // Battery voltage measurement
// #define BATTERY_PIN 3
// #define ADC_CHANNEL ADC1_GPIO3_CHANNEL
// #define ADC_MULTIPLIER 2.0

#ifdef __cplusplus
}
#endif

#endif // _VARIANT_TINYLORA_MV_GAME_
