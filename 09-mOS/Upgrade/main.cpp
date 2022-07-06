#include "mbed.h"

extern void bootloader(void);

int main() {
    bootloader();
    mbed_start_application(0xb400);
}
