#include "pico/stdlib.h"
#include "sensors.hpp"
#include "lcd.hpp"
#include <sstream>
#include <iomanip>

#define SECOND 1000000

std::string get_time_string() {
    uint32_t seconds = time_us_64() / SECOND; // 32-bit is overflow in 136 years so we should be safe.
    std::ostringstream oss{};
    oss << CLOCK
        << ' ' 
        << std::setw(2) 
        << std::setfill('0') 
        << (seconds / 60) 
        << ':' 
        << std::setw(2) 
        << std::setfill('0') 
        << (seconds % 60);
    return oss.str();
}

int main() {
    stdio_init_all();

    LCD lcd1{0x24};
    LCD lcd2{0x27};

    uint64_t current_time{};
    uint64_t last_time_update{};

    // Never-ending superloop
    while (true) {
        current_time = time_us_64();
        if (current_time - last_time_update >= SECOND) {
            last_time_update = current_time;
            lcd1.write_line_center(get_time_string(), 1);
        }
        get_sensors()->tick(lcd1, lcd2);
    }
}