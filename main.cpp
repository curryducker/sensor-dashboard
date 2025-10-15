#include "pico/stdlib.h"
#include "sensors.hpp"

int main() {
    stdio_init_all();

    // Never-ending superloop
    while (true) {
        get_sensors()->tick();
    }
}