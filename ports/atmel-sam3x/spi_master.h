#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <stdbool.h>
#include <stdint.h>

void spi_master_init(void);
void spi_master_set_frequency(uint32_t freq_hz);
void spi_master_set_csat(bool enable);
uint8_t spi_master_transfer(uint8_t out);
void spi_master_transfer_bytes(const uint8_t *out, uint8_t *in, uint32_t len);

#endif
