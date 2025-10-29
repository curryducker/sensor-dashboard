#include "sensors.hpp"

static SensorShield sensors{};

SensorShield::SensorShield()
{
    gpio_init_mask((1 << ALS_INT_GPIO) | (1 << TEMP_INT_GPIO)); // Default is input.

    // Setup interrupt callback
    gpio_set_irq_enabled_with_callback(
        ALS_INT_GPIO,
        GPIO_IRQ_EDGE_FALL,
        true,
        gpio_callback);
    gpio_set_irq_enabled(TEMP_INT_GPIO, GPIO_IRQ_EDGE_FALL, true);

    // Setup I2C bus
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    i2c_init(i2c0, BAUD);

    // Setup sensors
    setup_als();
    setup_temp();
}

SensorShield::~SensorShield()
{
    i2c_deinit(i2c0);
}

inline void SensorShield::trigger_als()
{
    als_int_trig = true;
}

inline void SensorShield::trigger_temp()
{
    temp_int_trig = true;
}

void SensorShield::tick()
{
    if (als_int_trig)
    {
        als_int_trig = false;
        als_callback();
    }
    if (temp_int_trig)
    {
        temp_int_trig = false;
        temp_callback();
    }
}

void SensorShield::write_i2c(uint8_t addr, uint8_t reg, uint8_t *val, size_t len) const
{
    uint8_t *buffer = new uint8_t[len + 1];
    buffer[0] = reg;
    for (size_t i = 0; i < len; i++)
    {
        buffer[i + 1] = val[i];
    }

    i2c_write_blocking(i2c0, addr, buffer, len + 1, false);

    delete[] buffer;
}

void SensorShield::write_i2c(uint8_t addr, uint8_t reg, uint8_t val) const
{
    write_i2c(addr, reg, &val, 1);
}

void SensorShield::als_set_thres(uint8_t reg, uint32_t val) const
{
    write_i2c(ALS_ADDRESS, reg, val & 0xFF);
    write_i2c(ALS_ADDRESS, reg + 1, (val >> 8) & 0xFF);
    write_i2c(ALS_ADDRESS, reg + 2, (val >> 16) & 0xF);
}

void SensorShield::setup_als() const
{
    // Pull up for the interrupt
    gpio_pull_up(ALS_INT_GPIO);
    write_i2c(ALS_ADDRESS, ALS_REG_MAIN_CTRL, 0x02); // ALS_EN = 1
    // Highest Precision = 20 bit, Max rate will be 400 ms.
    //
    // When the measurement repeat rate is programmed to be faster than possible for the specified
    // ADC measurement time, the repeat rate will be lower than programmed (maximum speed).
    //
    // So we'll just ignore the rate setting and set it to the maximum.
    write_i2c(ALS_ADDRESS, ALS_REG_MEAS_RATE, 0x00);
    // Set gain to 18x to focus on lower range, we don't want a simple shadow to be detected as night.
    write_i2c(ALS_ADDRESS, ALS_REG_GAIN, 0x04);
    write_i2c(ALS_ADDRESS, ALS_REG_INT_CFG, 0x14);         // ALS channel, threshold mode, enabled.
    write_i2c(ALS_ADDRESS, ALS_REG_INT_PERSISTENCE, 0x10); // 2 consecutive values triggers interrupt.

    // Set Threshold
    als_set_thres(ALS_REG_UP_THRES, ALS_UPPER_THRES);
    als_set_thres(ALS_REG_LOW_THRES, ALS_LOWER_THRES);
}

void SensorShield::setup_temp() const
{
    gpio_pull_up(TEMP_INT_GPIO);
    write_i2c(TEMP_ADDRESS, TEMP_REG_CONF, 0x02); // Set OS Interrupt mode

    // Set overheat warning threshold
    constexpr int lower_thres = (TEMP_THYS_VAL * 2) << 7;
    constexpr int higher_thres = (TEMP_TOS_VAL * 2) << 7;
    uint8_t lower_buffer[2] = {*((uint8_t*)&lower_thres + 1), *((uint8_t*)&lower_thres)};
    uint8_t higher_buffer[2] = {*((uint8_t*)&higher_thres + 1), *((uint8_t*)&higher_thres)};
    write_i2c(TEMP_ADDRESS, TEMP_REG_THYS, lower_buffer, 2);
    write_i2c(TEMP_ADDRESS, TEMP_REG_TOS, higher_buffer, 2);

    i2c_write_blocking(i2c0, TEMP_ADDRESS, TEMP_REG_TEMP, 1, false); // Set pointer
}

void SensorShield::als_callback()
{
    // Read ALS.
    uint32_t val{};
    get_sensors()->als_read(&val);
    printf("\n%d\n", val);
    printf("0x%05X", val);
    std::cout << std::endl;

    if (val < ALS_LOWER_THRES)
    {
        // Set to only upper threshold.
        als_set_thres(ALS_REG_UP_THRES, ALS_UPPER_THRES);
        als_set_thres(ALS_REG_LOW_THRES, 0);
        night = true;
    }
    else if (val > ALS_UPPER_THRES)
    {
        // Set to only lower threshold.
        als_set_thres(ALS_REG_UP_THRES, 0xFFFFF);
        als_set_thres(ALS_REG_LOW_THRES, ALS_LOWER_THRES);
        night = false;
    }
    std::cout << "Night: " << night << std::endl;

    // Read status to reset interrupt.
    get_sensors()->als_status();
}

void SensorShield::temp_callback()
{
    // Read ALS.
    get_sensors()->temp_read();
    printf("\nTemperature: %f\n", temp_value);
    std::cout << std::endl;

    if (temp_value >= TEMP_TOS_VAL) {
        // Set overheat warning
    } else if (temp_value <= TEMP_THYS_VAL) {
        // Remove overheat warning
    }
}

void SensorShield::als_read(uint32_t *val) const
{
    // My love/hate relationship with endianness loves the fact that Pico is little endian right now.
    // This means i can just give the address to a 32-bit integer without worrying about the 12 unused bits.
    *val = 0; // Reset value, removes garbage from the unused byte in an event where the variable wasn't initialized.
    constexpr uint8_t reg[] = {ALS_REG_DATA};
    i2c_write_blocking(i2c0, ALS_ADDRESS, reg, 1, true);
    // 20 bits, so 3 bytes
    i2c_read_blocking(i2c0, ALS_ADDRESS, (uint8_t *)val, 3, false);
}

void SensorShield::als_status() const
{
    constexpr uint8_t reg[] = {ALS_REG_STATUS};
    uint8_t val{};
    i2c_write_blocking(i2c0, ALS_ADDRESS, reg, 1, true);
    i2c_read_blocking(i2c0, ALS_ADDRESS, &val, 1, false);
    printf("Status: 0x%02X", val);
    std::cout << std::endl;
}

void SensorShield::temp_read()
{
    uint16_t buffer{};
    // No need to write a register because of preset pointer register, which has been set at config.
    i2c_read_blocking(i2c0, TEMP_ADDRESS, (uint8_t*)&buffer, sizeof(buffer), false);
    
    int decimal = 0; // Reset Value
    // The sensor is big endian, we need to do some memory modification
    // We will first cast to uint8_t* so we can do per-byte arithmetic.
    // Then we will cast to void* so we can switch some bytes.
    std::memcpy((void*)&decimal, (void*)((uint8_t*)&buffer + 1), 1);
    std::memcpy((void*)((uint8_t*)&decimal + 1), (void*)&buffer, 1);
    decimal = decimal >> 5; // Shift 5 places
    // Conversion from decimal value to degrees is stated in section 7.4.3 of the datasheet.
    // The sensor has a precision of 0.125 degrees celsius, so every value is 0.125 degrees.
    temp_value = decimal * 0.125f;
}

SensorShield *get_sensors()
{
    return &sensors;
}

void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == ALS_INT_GPIO && (events & GPIO_IRQ_EDGE_FALL))
    {
        get_sensors()->trigger_als();
    }
    else if (gpio == TEMP_INT_GPIO && (events & GPIO_IRQ_EDGE_FALL))
    {
        get_sensors()->trigger_temp();
    }
}
