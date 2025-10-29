#ifndef ALS_HPP
#define ALS_HPP

#include "pico/stdlib.h"
#include <cstring>
#include "hardware/i2c.h"
#include <iostream>

// GENERAL I2C
#define I2C_SDA 4
#define I2C_SCL 5
#define BAUD 100000 // Both chips support a clockrate up to 400 kHz so we'll take 100 kHz.

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

#define TEMP_THYS_VAL 22
#define TEMP_TOS_VAL 25

#define TEMP_INT_GPIO 7

// SENSOR SHIELD CLASS

class SensorShield
{
public:
    SensorShield();
    ~SensorShield();
    inline void trigger_als();
    inline void trigger_temp();
    void tick();

private:
    void write_i2c(uint8_t addr, uint8_t reg, uint8_t *val, size_t len) const;
    void write_i2c(uint8_t addr, uint8_t reg, uint8_t val) const;

    void als_set_thres(uint8_t reg, uint32_t val) const;

    void setup_als() const;
    void setup_temp() const;

    void als_callback();
    void temp_callback();

    void als_read(uint32_t *val) const;
    void als_status() const;
    void temp_read();

    bool night = false;
    
    // Initialize to true for initial reading
    volatile bool als_int_trig = true;
    volatile bool temp_int_trig = true;

    float temp_value{};
};

SensorShield *get_sensors();

void gpio_callback(uint gpio, uint32_t events);

#endif // ALS_HPP