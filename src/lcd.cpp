#include "lcd.hpp"

LCD::LCD(uint8_t address, uint8_t rows, uint8_t columns) : address_{address}, rows_{rows}, columns_{columns}
{
    // Setup I2C bus, if not done already
    if (I2C_CHANNEL->hw->enable == 0) {
        gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
        i2c_init(I2C_CHANNEL, BAUD);
    }
    if (rows_ > MAX_ROWS) rows_ = MAX_ROWS;
    if (columns_ > MAX_COLUMNS) columns_ = MAX_COLUMNS;
    // "The busy state lasts for 10 ms after VCC rises to 4.5 V."
    // We will wait 11 then.
    while (time_us_64() < 11000) {
        tight_loop_contents();
    }

    // The display initializes in 8-bit mode.
    // It supports a single 4-bit write to set the display to 4-bit mode (See table 12, datasheet)
    write_command(0x2);
    // From now on, for any instructions we will use table 6 from the datasheet.

    // Function set: 4 bit mode, 2 lines, 5x8 font.
    // We actually support 4 lines, see write_character function.
    write_command(0x28);
    // Display control: Display ON, Cursor OFF.
    write_command(0x0C);
    // No need to set entry mode.
    
    // Register Custom Characters.
    register_characters();
    // Clear display.
    clear();
}

LCD::LCD(uint8_t address) : LCD{address, DEFAULT_ROWS, DEFAULT_COLUMNS} {}

LCD::LCD() : LCD{DEFAULT_ADDRESS} {}

inline void LCD::clear() const
{
    write_command(0x1);
}

void LCD::set_cursor(uint8_t row, uint8_t column) const
{
    if (row >= rows_ || column >= columns_) return;
    // The HD44780 supports 2 lines of 40 characters (See section about DDRAM in datasheet).
    // The first line starts at DDRAM address 0x00, the second at 0x40.
    // A display of 4x20 splits those lines (rows 0-3).
    // This means that row 0 and 2 use the addresses of line 1.
    // Row 1 and 3 use line 2.

    // For the offsets, this means:
    // Row 1 and 3 have an offset of 0x40 (Line 2).
    // Row 2 and 3 have an extra offset of the amount of columns (+20).
    uint8_t ddram_address = column + (row % 2 == 0 ? 0x00 : 0x40) + (row >= 2 ? columns_ : 0);
    write_command(0x80 | ddram_address); // Set DDRAM Address (Cursor).
}

void LCD::write_character(char c, uint8_t row, uint8_t column) const
{
    if (row >= rows_ || column >= columns_) return;
    set_cursor(row, column);
    write_data(c);
}

void LCD::write_line(const std::string &string, uint8_t row, uint8_t offset) const
{
    if (string.length() + offset > columns_ || row > rows_) return;
    uint8_t i = 0;
    while (i < offset) {
        write_character(' ', row, i);
        i++;
    }
    for (const char& c : string) {
        write_character(c, row, i);
        i++;
    }
    while (i < columns_) {
        write_character(' ', row, i);
        i++;
    }
}

void LCD::write_line_center(const std::string &string, uint8_t row) const
{
    if (string.length() > columns_ || row > rows_) return;
    write_line(string, row, (columns_ - string.length()) / 2);
}

void LCD::write_i2c(uint8_t data, bool rs) const
{
    // We will always write so we won't change bit 1 (RW = 0)
    constexpr uint8_t en = 0x4; // P2 is EN, so 0x4.
    constexpr uint8_t bl = 0x8; // P3 is backlight, which will stay on, so 0x8.

    // 4-bit mode, so send in 2 parts.
    uint8_t data_1 = (data & 0xF0); // Upper 4 bits
    uint8_t data_2 = ((data << 4) & 0xF0); // Lower 4 bits, shifted to upper 4.

    uint8_t part_1 = data_1 | en | bl | rs; // Upper 4 bits, EN = 1;
    uint8_t part_2 = (data_1 & ~en) | bl | rs; // Upper 4 bits, EN = 0;
    uint8_t part_3 = data_2 | en | bl | rs; // Lower 4 bits, EN = 1;
    uint8_t part_4 = (data_2 & ~en) | bl | rs; // Lower 4 bits, EN = 0;

    i2c_write_blocking(I2C_CHANNEL, address_, &part_1, 1, false);
    i2c_write_blocking(I2C_CHANNEL, address_, &part_2, 1, false);
    // The biggest instruction execution time is 1.52ms according to table 6.
    // We will wait 2ms to be safe (and to avoid checking the busy flag).
    sleep_ms(2);
    i2c_write_blocking(I2C_CHANNEL, address_, &part_3, 1, false);
    i2c_write_blocking(I2C_CHANNEL, address_, &part_4, 1, false);
    sleep_ms(2);
}

inline void LCD::write_command(uint8_t command) const
{
    write_i2c(command, false); // RS = 0 means write command.
}

inline void LCD::write_data(uint8_t data) const
{
    write_i2c(data, true); // RS = 1 means write data.
}

void LCD::register_character(char location, uint64_t character)
{
    if (location > 7) return; // Max 8 Characters.
    write_command(0x40 | (location * 8)); // Set CGRAM, 8 bytes per character (5x8 font)
    for (int i = 7; i >= 0; i--)
    {
        write_data((character >> (8 * i)) & 0xFF); // Send character per byte, MSB first.
    }
}

void LCD::register_characters()
{
    // The following diagram shows how a character turns into a 64-bit integer.
    // https://curryducker.nl/assets/school/character.png
    register_character(CLOCK, 0x00000E1515130E00);
    register_character(DAY, 0x00150E1B0E150000);
    register_character(NIGHT, 0x00040C080C060000);
    register_character(OVERHEATING, 0x02140D0E1A13190E);
}
