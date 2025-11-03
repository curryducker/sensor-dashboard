#ifndef SENSORS_HPP
#define SENSORS_HPP

#include "pico/stdlib.h"
#include <cstring>
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include <iostream>
#include "dashboard_i2c.hpp"
#include "lcd.hpp"
#include <sstream>

// ALS
#define ALS_ADDRESS 0x52

#define ALS_REG_MAIN_CTRL 0x00
#define ALS_REG_MEAS_RATE 0x04
#define ALS_REG_GAIN 0x05
#define ALS_REG_STATUS 0x07
#define ALS_REG_DATA 0x0D // 3 Bytes, 0x0D - 0x0F
#define ALS_REG_INT_CFG 0x19
#define ALS_REG_INT_PERSISTENCE 0x1A
#define ALS_REG_UP_THRES 0x21
#define ALS_REG_LOW_THRES 0x24

#define ALS_UPPER_THRES 1500
#define ALS_LOWER_THRES 1000

#define ALS_INT_GPIO 6

// TEMP SENSOR
#define TEMP_ADDRESS 0x49

#define TEMP_REG_TEMP 0x00
#define TEMP_REG_CONF 0x01
#define TEMP_REG_THYS 0x02
#define TEMP_REG_TOS 0x03

#define TEMP_THYS_VAL 30
#define TEMP_TOS_VAL 40

#define TEMP_INT_GPIO 7

// LEDS
#define CLK 18
#define MOSI 19
#define MISO 16
#define CS 17

#define LED_MSG_SIZE 3
#define COMMAND_BYTE_LED 0x40
#define REGISTER_LED 0x9
#define REGISTER_LED_SETUP 0x0

// IO thats not on the Sensor Shield board.
#define BUTTON_GPIO 20
#define BIG_LED_PIN 21

// SENSOR SHIELD CLASS

class SensorShield
{
public:
    SensorShield();
    ~SensorShield();
    inline void trigger_als();
    inline void trigger_temp();
    inline void button_pressed();
    void tick(LCD& lcd1, LCD& lcd2);

private:
    void write_i2c(uint8_t addr, uint8_t reg, uint8_t *val, size_t len) const;
    void write_i2c(uint8_t addr, uint8_t reg, uint8_t val) const;
    void write_to_io(uint8_t reg, uint8_t val) const;
    void write_leds(uint8_t val) const;

    inline void set_cs(uint8_t cs, bool high) const;

    void als_set_thres(uint8_t reg, uint32_t val) const;

    void setup_als() const;
    void setup_temp() const;
    void setup_leds() const;

    void als_callback(LCD& lcd);
    void temp_callback(LCD& lcd);
    void update_leds();

    void als_read();
    void als_status() const;
    void temp_read();
    
    // Initialize to true for initial reading
    volatile bool als_int_trig_ = true;
    volatile bool temp_int_trig_ = true;

    volatile bool button_int_trig_ = true;

    uint32_t als_value_{};
    float temp_value_{};

    bool led_src_als_ = true;
};

SensorShield *get_sensors();

void gpio_callback(uint gpio, uint32_t events);

#endif // SENSORS_HPP