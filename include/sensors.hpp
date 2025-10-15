#ifndef SENSORS_HPP
#define SENSORS_HPP

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <iostream>
#include "dashboard_i2c.hpp"

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

// SENSOR SHIELD CLASS

class SensorShield
{
public:
    SensorShield();
    ~SensorShield();
    inline void trigger_als();
    void tick();

private:
    void write_i2c(uint8_t addr, uint8_t reg, uint8_t *val, size_t len) const;
    void write_i2c(uint8_t addr, uint8_t reg, uint8_t val) const;

    void als_set_thres(uint8_t reg, uint32_t val) const;

    void setup_als() const;

    void als_callback();

    void als_read(uint32_t *val) const;
    void als_status() const;

    bool night_ = false;
    
    // Initialize to true for initial reading
    volatile bool als_int_trig_ = true;
};

SensorShield *get_sensors();

void gpio_callback(uint gpio, uint32_t events);

#endif // SENSORS_HPP