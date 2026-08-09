/*
 * SD/MMC card protocol over SPI for the SAM3X8E.
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
#include "modpinmap.h"
#include "modpin.h"
#include "time.h"
#include "extmod/vfs.h"
#include "spi_master.h"
#include "sdcard_spi.h"

#define SDCARD_INIT_CLOCK_HZ    400000UL
#define SDCARD_CMD_TIMEOUT_MS   100
#define SDCARD_DATA_TIMEOUT_MS  100
#define SDCARD_INIT_TIMEOUT_MS  1000

// Single card instance: static configuration, set by sdcard_init().
static sdcard_config_t sdcard_config;
static bool sdcard_block_addressing = false;
static bool sdcard_initialised = false;

// ---- chip select ----
// GPIO mode drives the CS pin directly. Hardware-CS mode relies on
// SPI_CSR_CSAAT + SPI_TDR_LASTXFER (Task 7); until then use_hw_cs is
// treated as GPIO by sdcard_cs_low/high, which Task 7 replaces.

static void sdcard_cs_low(void) {
    set_pin(sdcard_config.cs_pin, 0);
}

static void sdcard_cs_high(void) {
    set_pin(sdcard_config.cs_pin, 1);
}

// ---- CRC ----

static uint8_t sdcard_crc7(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        for (uint8_t j = 8; j-- != 0;) {
            crc = (uint8_t)((crc << 1) | ((data[i] >> j) & 1));
            if (crc & 0x80) {
                crc ^= 0x89;
            }
        }
    }
    // account for the 7 zero bits appended by the x^7 term of M(x)*x^7 mod G(x)
    for (uint8_t j = 0; j < 7; j++) {
        crc = (uint8_t)(crc << 1);
        if (crc & 0x80) {
            crc ^= 0x89;
        }
    }
    return crc & 0x7f;
}

static uint16_t sdcard_crc16(const uint8_t *data, uint32_t len) {
    uint16_t crc = 0;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
        }
    }
    return crc;
}

// ---- command / response ----
// Sends a 6-byte command frame; returns R1 (0xff on timeout). CRC is only
// required by the card for CMD0/CMD8; for all others a dummy CRC byte works.

static uint8_t sdcard_cmd(uint8_t cmd, uint32_t arg, bool has_crc) {
    uint8_t buf[6];
    buf[0] = 0x40 | cmd;
    buf[1] = (uint8_t)(arg >> 24);
    buf[2] = (uint8_t)(arg >> 16);
    buf[3] = (uint8_t)(arg >> 8);
    buf[4] = (uint8_t)(arg >> 0);
    buf[5] = has_crc ? ((sdcard_crc7(buf, 5) << 1) | 1) : 0x01;
    spi_master_transfer_bytes(buf, NULL, 6);

    // read response: wait for a byte with MSB clear (R1)
    uint32_t start = time_millis();
    uint8_t r1;
    do {
        r1 = spi_master_transfer(0xff);
        if ((r1 & 0x80) == 0) {
            return r1;
        }
    } while (time_millis() - start < SDCARD_CMD_TIMEOUT_MS);
    return 0xff;
}

// ---- data token wait ----

static uint8_t sdcard_wait_data_token(uint8_t token) {
    uint32_t start = time_millis();
    uint8_t b;
    do {
        b = spi_master_transfer(0xff);
        if (b == token) {
            return b;
        }
    } while (time_millis() - start < SDCARD_DATA_TIMEOUT_MS);
    return 0xff;
}

// Wait for the card to finish an internal operation (busy = MISO low).
static bool sdcard_wait_not_busy(void) {
    uint32_t start = time_millis();
    while (spi_master_transfer(0xff) == 0x00) {
        if (time_millis() - start > SDCARD_DATA_TIMEOUT_MS) {
            return false;
        }
    }
    return true;
}

// ---- public API ----

bool sdcard_init(sdcard_config_t *config) {
    sdcard_config = *config;
    sdcard_block_addressing = false;
    sdcard_initialised = false;

    // CS as GPIO output, high (deselected)
    ini_pin(sdcard_config.cs_pin, OUTPUT);
    set_pin(sdcard_config.cs_pin, 1);

    // SPI at init clock speed
    spi_master_init();
    spi_master_set_frequency(SDCARD_INIT_CLOCK_HZ);

    // 80 clock cycles with CS high (power-up pulse)
    uint8_t dummy[10];
    for (int i = 0; i < 10; i++) {
        dummy[i] = 0xff;
    }
    spi_master_transfer_bytes(dummy, NULL, 10);

    // keep CS low for the whole init sequence
    sdcard_cs_low();

    // CMD0: go to idle (CRC is mandatory here); retry a few times as the
    // first command after power-up can be lost
    bool idle = false;
    for (int attempt = 0; attempt < 5 && !idle; attempt++) {
        idle = (sdcard_cmd(0, 0x00000000, true) == 0x01);
    }
    if (!idle) {
        sdcard_cs_high();
        return false;
    }

    // CMD8: probe for SD v2; 0x05 (illegal command) means SD v1
    uint8_t r1 = sdcard_cmd(8, 0x000001aa, true);
    if (r1 == 0x01) {
        uint8_t echo[4];
        spi_master_transfer_bytes(NULL, echo, 4);
        if (echo[2] != 0x01 || echo[3] != 0xaa) {
            sdcard_cs_high();
            return false;
        }
    } else if (r1 != 0x05) {
        sdcard_cs_high();
        return false;
    }

    // ACMD41 loop: host capacity support bit first, retry without HCS for v1
    bool hcs = true;
    r1 = 0xff;
    uint32_t start = time_millis();
    do {
        r1 = sdcard_cmd(55, 0, false); // APP_CMD
        if (r1 > 1) {
            sdcard_cs_high();
            return false;
        }
        r1 = sdcard_cmd(41, hcs ? 0x40000000 : 0x00000000, false); // SEND_OP_COND
        if (r1 == 0x05) {
            hcs = false; // SD v1 does not support HCS
        } else if (r1 > 1) {
            sdcard_cs_high();
            return false;
        }
    } while (r1 != 0x00 && time_millis() - start < SDCARD_INIT_TIMEOUT_MS);
    if (r1 != 0x00) {
        sdcard_cs_high();
        return false;
    }

    // CMD58: read OCR; CCS (bit 30) indicates block addressing (SDHC/SDXC)
    if (sdcard_cmd(58, 0, false) == 0x00) {
        uint8_t ocr[4];
        spi_master_transfer_bytes(NULL, ocr, 4);
        sdcard_block_addressing = (ocr[0] & 0x40) != 0;
    }

    // CMD16: set block length to 512 (ignored by SDHC/SDXC)
    if (sdcard_cmd(16, SDCARD_BLOCK_SIZE, false) != 0x00) {
        sdcard_cs_high();
        return false;
    }

    sdcard_cs_high();

    // switch to full-speed clock
    spi_master_set_frequency(sdcard_config.spi_freq);

    sdcard_initialised = true;
    return true;
}

bool sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks) {
    if (!sdcard_initialised) {
        return false;
    }
    sdcard_cs_low();

    uint32_t addr = sdcard_block_addressing ? block_num : block_num * SDCARD_BLOCK_SIZE;

    if (num_blocks == 1) {
        // CMD17: single block read
        if (sdcard_cmd(17, addr, false) != 0x00) {
            sdcard_cs_high();
            return false;
        }
        if (sdcard_wait_data_token(0xfe) != 0xfe) {
            sdcard_cs_high();
            return false;
        }
        spi_master_transfer_bytes(NULL, dest, SDCARD_BLOCK_SIZE);
        spi_master_transfer_bytes(NULL, NULL, 2); // CRC16
    } else {
        // CMD18: multiple block read (multi-block body added in Task 4)
        sdcard_cs_high();
        return false;
    }

    sdcard_cs_high();
    return true;
}

bool sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks) {
    if (!sdcard_initialised) {
        return false;
    }
    sdcard_cs_low();

    uint32_t addr = sdcard_block_addressing ? block_num : block_num * SDCARD_BLOCK_SIZE;

    if (num_blocks == 1) {
        // CMD24: single block write
        if (sdcard_cmd(24, addr, false) != 0x00) {
            sdcard_cs_high();
            return false;
        }
        spi_master_transfer(0xfe); // data start token
        spi_master_transfer_bytes(src, NULL, SDCARD_BLOCK_SIZE);
        uint16_t crc = sdcard_crc16(src, SDCARD_BLOCK_SIZE);
        spi_master_transfer((uint8_t)(crc >> 8));
        spi_master_transfer((uint8_t)(crc & 0xff));

        // data response: 0bxxx0SSS1, accepted = 0x05
        uint8_t resp = spi_master_transfer(0xff);
        if ((resp & 0x1f) != 0x05) {
            sdcard_cs_high();
            return false;
        }
        if (!sdcard_wait_not_busy()) {
            sdcard_cs_high();
            return false;
        }
    } else {
        // CMD25: multiple block write (multi-block body added in Task 4)
        sdcard_cs_high();
        return false;
    }

    sdcard_cs_high();
    return true;
}

uint32_t sdcard_ioctl(uint32_t cmd, uint32_t arg) {
    switch (cmd) {
        case BP_IOCTL_INIT:
            return sdcard_initialised ? 0 : 1;
        case BP_IOCTL_DEINIT:
            sdcard_initialised = false;
            return 0;
        case BP_IOCTL_SYNC:
            return 0;
        case BP_IOCTL_SEC_COUNT:
            return 0; // implemented in Task 4 via CSD parse
        case BP_IOCTL_SEC_SIZE:
            return SDCARD_BLOCK_SIZE;
        default:
            return 0;
    }
}

bool sdcard_is_present(void) {
    if (!sdcard_initialised) {
        return false;
    }
    sdcard_cs_low();
    uint8_t r1 = sdcard_cmd(13, 0, false); // SEND_STATUS
    sdcard_cs_high();
    return r1 != 0xff;
}
