// For the full instruction set, see datasheet
// https://www.alldatasheet.com/datasheet-pdf/download/63673/HITACHI/HD44780.html
#ifndef LCD_HPP
#define LCD_HPP

#include "pico/stdlib.h"
#include <string>
#include "hardware/i2c.h"
#include "dashboard_i2c.hpp"

#define DEFAULT_ADDRESS 0x27
#define DEFAULT_ROWS 4
#define DEFAULT_COLUMNS 20
#define MAX_ROWS 4
#define MAX_COLUMNS 20

#define CLOCK '\1'
#define DAY '\2'
#define NIGHT '\3'
#define OVERHEATING '\4'

class LCD {
    public:
    LCD(uint8_t address, uint8_t rows, uint8_t columns);
    LCD(uint8_t address);
    LCD();

    inline void clear() const;
    void set_cursor(uint8_t row, uint8_t column) const;
    void write_character(char c, uint8_t row, uint8_t column) const;
    void write_line(const std::string& string, uint8_t row, uint8_t offset) const;
    void write_line_center(const std::string& string, uint8_t row) const;

    private:
    void write_i2c(uint8_t data, bool rs) const;
    inline void write_command(uint8_t command) const;
    inline void write_data(uint8_t data) const;

    void register_character(char location, uint64_t character);
    void register_characters();

    uint8_t address_{};
    uint8_t rows_{};
    uint8_t columns_{};
};

#endif // LCD_HPP