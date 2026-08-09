#ifndef SDCARD_SPI_H
#define SDCARD_SPI_H

#include <stdbool.h>
#include <stdint.h>

#define SDCARD_BLOCK_SIZE 512

typedef struct _sdcard_config_t {
    uint8_t cs_pin;     // board pin index (g_APinDescription) for chip select
    uint32_t spi_freq;  // data-rate SPI clock in Hz (init always uses 400 kHz)
    bool use_hw_cs;     // true = SPI NPCS0 hardware CS; false = GPIO CS
} sdcard_config_t;

bool sdcard_init(sdcard_config_t *config);
bool sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks);
bool sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks);
uint32_t sdcard_ioctl(uint32_t cmd, uint32_t arg);
bool sdcard_is_present(void);

#endif
