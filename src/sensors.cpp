#include "sensors.hpp"

static SensorShield sensors{};

SensorShield::SensorShield()
{
    gpio_init_mask((1 << ALS_INT_GPIO)); // Default is input.

    // Setup interrupt callback
    gpio_set_irq_enabled_with_callback(
        ALS_INT_GPIO,
        GPIO_IRQ_EDGE_FALL,
        true,
        gpio_callback);

    // Setup I2C bus, if not done already
    if (I2C_CHANNEL->hw->enable == 0) {
        gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
        i2c_init(I2C_CHANNEL, BAUD);
    }

    // Setup sensors
    setup_als();
}

SensorShield::~SensorShield() {}

inline void SensorShield::trigger_als()
{
    als_int_trig_ = true;
}

void SensorShield::tick()
{
    if (als_int_trig_)
    {
        als_int_trig_ = false;
        als_callback();
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

    i2c_write_blocking(I2C_CHANNEL, addr, buffer, len + 1, false);

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
        night_ = true;
    }
    else if (val > ALS_UPPER_THRES)
    {
        // Set to only lower threshold.
        als_set_thres(ALS_REG_UP_THRES, 0xFFFFF);
        als_set_thres(ALS_REG_LOW_THRES, ALS_LOWER_THRES);
        night_ = false;
    }
    std::cout << "Night: " << night_ << std::endl;

    // Read status to reset interrupt.
    get_sensors()->als_status();
}

void SensorShield::als_read(uint32_t *val) const
{
    // My love/hate relationship with endianness loves the fact that Pico is little endian right now.
    // This means i can just give the address to a 32-bit integer without worrying about the 12 unused bits.
    *val = 0; // Reset value, removes garbage from the unused byte in an event where the variable wasn't initialized.
    constexpr uint8_t reg[] = {ALS_REG_DATA};
    i2c_write_blocking(I2C_CHANNEL, ALS_ADDRESS, reg, 1, true);
    // 20 bits, so 3 bytes
    i2c_read_blocking(I2C_CHANNEL, ALS_ADDRESS, (uint8_t *)val, 3, false);
}

void SensorShield::als_status() const
{
    constexpr uint8_t reg[] = {ALS_REG_STATUS};
    uint8_t val{};
    i2c_write_blocking(I2C_CHANNEL, ALS_ADDRESS, reg, 1, true);
    i2c_read_blocking(I2C_CHANNEL, ALS_ADDRESS, &val, 1, false);
    printf("Status: 0x%02X", val);
    std::cout << std::endl;
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
}
