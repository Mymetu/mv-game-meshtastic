#include "variant.h"
#include <Arduino.h>

void earlyInitVariant()
{
#ifdef GPS_EN_EARLY_PULLDOWN
    // 1. Pull GPS EN LOW immediately → GPS module fully off
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, LOW);

    // 2. Pull GPS UART pins LOW to prevent leakage through
    //    IO protection diodes (even with EN=0, TX/RX can float)
    pinMode(GPS_TX_PIN, OUTPUT);
    digitalWrite(GPS_TX_PIN, LOW);
    pinMode(GPS_RX_PIN, OUTPUT);
    digitalWrite(GPS_RX_PIN, LOW);
#endif
}

void lateInitVariant()
{
#ifdef GPS_EN_EARLY_PULLDOWN
    // Wait until 3 seconds from boot, then re-enable GPS.
    // This gives the system time to stabilize (flash, LoRa, I2C,
    // buzzer startup tone, etc.) before GPS draws any current.
    static uint32_t bootMs = millis();
    uint32_t elapsed = millis() - bootMs;
    if (elapsed < 3000) {
        delay(3000 - elapsed);
    }

    // 1. Release UART pins first (GPS::createGps will reconfigure)
    pinMode(GPS_TX_PIN, INPUT);
    pinMode(GPS_RX_PIN, INPUT);

    // 2. Enable GPS power
    digitalWrite(PIN_GPS_EN, HIGH);
#endif
}
