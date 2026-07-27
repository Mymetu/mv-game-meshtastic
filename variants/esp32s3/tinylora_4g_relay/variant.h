#ifndef _VARIANT_TINYLORA_4G_RELAY_
#define _VARIANT_TINYLORA_4G_RELAY_

#ifdef __cplusplus
extern "C" {
#endif

/* LED (active high) */
#define LED_PIN 48
#define LED_STATE_ON 1

/* Button (BOOT) */
#define BUTTON_PIN 0
#define BUTTON_NEED_PULLUP

/* LoRa (LLCC68 / SX1262 / SX1268 with RXEN) */
#define USE_LLCC68
#define USE_SX1262
#define USE_SX1268

#define LORA_SCK 5
#define LORA_MISO 3
#define LORA_MOSI 6
#define LORA_CS 7

#define LORA_DIO0 RADIOLIB_NC
#define LORA_DIO1 16
#define LORA_DIO2 RADIOLIB_NC
#define LORA_BUSY 15
#define LORA_RESET 8

#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_BUSY
#define SX126X_RESET LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_RXEN 14

#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define TCXO_OPTIONAL

/* Battery (GPIO1, ADC1_CH1) */
#define BATTERY_PIN 1
#define ADC_CHANNEL ADC1_GPIO1_CHANNEL
#define ADC_MULTIPLIER 2.0f

/* I2C (reserved, not used on relay) */
#define WIRE_INTERFACES_COUNT 1
#define I2C_SDA 18
#define I2C_SCL 17

#ifdef __cplusplus
}
#endif
#endif // _VARIANT_TINYLORA_4G_RELAY_
