#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    // Never-ending superloop
    while (true) {
        tight_loop_contents();
    }
}