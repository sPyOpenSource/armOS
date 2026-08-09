/*
 * Register-level SPI0 master driver for the SAM3X8E.
 *
 * The ASF tree in this repository does not ship an SPI driver source, so this
 * file drives the SPI0 peripheral directly through its CMSIS registers.
 *
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2026
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdbool.h>
#include <stdint.h>

#include "asf.h"
#include "spi_master.h"

void spi_master_init(void) {
    // Enable SPI0 peripheral clock
    pmc_enable_periph_clk(ID_SPI0);

    // Configure SPI0 pins: PA25=MISO, PA26=MOSI, PA27=SPCK (peripheral A)
    pio_configure(PIOA, PIO_PERIPH_A,
        PIO_PA25A_SPI0_MISO | PIO_PA26A_SPI0_MOSI | PIO_PA27A_SPI0_SPCK,
        PIO_DEFAULT);

    // Software reset SPI0
    SPI0->SPI_CR = SPI_CR_SWRST;

    // Master mode, mode fault detection disabled, PCS0 selected
    SPI0->SPI_MR = SPI_MR_MSTR | SPI_MR_MODFDIS | SPI_MR_PCS(0);

    // Enable SPI0
    SPI0->SPI_CR = SPI_CR_SPIEN;
}

static bool spi_master_csat = false;

void spi_master_set_frequency(uint32_t freq_hz) {
    uint32_t pclk = sysclk_get_peripheral_hz();
    uint32_t scbr = pclk / freq_hz;
    if (scbr < 1) {
        scbr = 1;
    } else if (scbr > 0xff) {
        scbr = 0xff;
    }
    // 8-bit transfers, SPI mode 0, baud-rate divisor for CS0;
    // optionally keep CS asserted after each byte (hardware-CS mode)
    uint32_t csr = SPI_CSR_BITS_8_BIT | SPI_CSR_SCBR(scbr);
    if (spi_master_csat) {
        csr |= SPI_CSR_CSAAT;
    }
    SPI0->SPI_CSR[0] = csr;
}

void spi_master_set_csat(bool enable) {
    spi_master_csat = enable;
    uint32_t csr = SPI0->SPI_CSR[0];
    if (enable) {
        csr |= SPI_CSR_CSAAT;
    } else {
        csr &= ~SPI_CSR_CSAAT;
    }
    SPI0->SPI_CSR[0] = csr;
}

uint8_t spi_master_transfer(uint8_t out) {
    // Wait for transmit buffer empty
    while (!(SPI0->SPI_SR & SPI_SR_TDRE)) {
    }
    SPI0->SPI_TDR = out;
    // Wait for receive data ready
    while (!(SPI0->SPI_SR & SPI_SR_RDRF)) {
    }
    return (uint8_t)SPI0->SPI_RDR;
}

void spi_master_transfer_bytes(const uint8_t *out, uint8_t *in, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uint8_t b = spi_master_transfer(out ? out[i] : 0xff);
        if (in) {
            in[i] = b;
        }
    }
}
