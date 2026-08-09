# SAM3X SPI SD Card Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add external SD card support via SPI0 to the atmel-sam3x (Arduino Due) MicroPython port, enabling FAT filesystem file storage from Python (`open()`, `os.mount`).

**Architecture:** A register-level SPI0 master driver (the ASF tree has no SPI driver, so we write one against CMSIS registers), an SD/MMC protocol layer over SPI (init, single/multi-block read/write, ioctl), and a `machine.SDCard` block-device type that MicroPython's existing VFS/FATFS (`extmod/vfs_fat`, `lib/oofatfs`) mounts via `os.mount(os.VfsFat(sd), "/sd")`. VFS/FATFS/os/io are not currently enabled in this port, so Task 1 enables that infrastructure first.

**Tech Stack:** C99, ARM Cortex-M3 (SAM3X8E), GCC 4.8.3 arm-none-eabi (hardcoded in Makefile), ASF CMSIS headers + `pio`/`pmc`/`sysclk` drivers, `lib/oofatfs` (ChaN FATFS), MicroPython `extmod/vfs_fat*`.

## Global Constraints

- SPI0 only. Pins: PA25 (MISO), PA26 (MOSI), PA27 (SPCK) — peripheral A. CS configurable: default PA28/NPCS0 (board pin 77), any GPIO, or SPI hardware NPCS0 via CSAAT/LASTXFER.
- Block size 512 bytes. Init clock ≤ 400 kHz; data clock default 25 MHz (SCBR divisor from `sysclk_get_peripheral_hz()`).
- Card detection is polling only (no CD pin). No DMA.
- Toolchain: `/Users/xuyi/Library/Arduino15/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-` (already `CROSS_COMPILE` in Makefile).
- Build from `ports/atmel-sam3x/` with `make -j8`; flash via `make upload port=<serial>`. Build must stay warning-free; the tree already builds (verified, firmware.bin 173 KB text).
- Follow port conventions: C99, `#include "py/headers/..."`, tab indentation, port-level type headers in `headers/`.
- **Spec deviations** (vs. `docs/superpowers/specs/2026-08-08-sam3x-spi-sdcard-design.md`, discovered during research — the plan supersedes the spec):
  1. No ASF SPI driver source exists in this tree → implement register-level SPI0 master in a new `spi_master.c` (spec's "existing ASF driver" was wrong).
  2. The port has no VFS/FATFS/os/io enabled at all → Task 1 adds the whole filesystem infrastructure.
  3. There is no `machine.SPI` class → constructor is `SDCard(cs_pin[, freq[, use_hw_cs]])`, not `SDCard(SPI(0), ...)`.
  4. Correct mount API is `os.mount(os.VfsFat(sd), "/sd")`, not `os.mount(sd, "/sd")` (the mounted object must be `mp_fat_vfs_type`, which has the `mount` method).
  5. CMD2/CMD3/CMD7 are unnecessary in SPI mode (CS selects the single card) and are omitted.

## File Structure

**New files:**
- `ports/atmel-sam3x/moduos.c` — `os` module exposing `mount`/`umount`/`VfsFat`/`listdir`/etc.
- `ports/atmel-sam3x/fatfs_port.c` — `get_fattime()` (required by `lib/oofatfs/ff.c`).
- `ports/atmel-sam3x/spi_master.h` / `spi_master.c` — register-level SPI0 master (replaces nonexistent ASF driver).
- `ports/atmel-sam3x/sdcard_spi.h` / `sdcard_spi.c` — SD/MMC protocol over SPI (init, block I/O, ioctl, present).
- `ports/atmel-sam3x/modsdcard.c` — `machine.SDCard` MicroPython type (block-device protocol).
- `ports/atmel-sam3x/headers/sdcard.h` — declares `pyb_sdcard_type` for `modpyb.h`.

**Modified files:**
- `ports/atmel-sam3x/headers/mpconfigport.h` — enable `MICROPY_VFS`, `MICROPY_VFS_FAT`, `MICROPY_READER_VFS`, `MICROPY_PY_IO`, `MICROPY_PY_IO_FILEIO`; register `uos`/`os`/`io` modules; alias `mp_import_stat`.
- `ports/atmel-sam3x/Makefile` — add new sources to `SRC_C`.
- `ports/atmel-sam3x/main.c` — delegate `mp_builtin_open` to `mp_vfs_open`; drop `mp_lexer_new_from_file`/`mp_import_stat` stubs.
- `ports/atmel-sam3x/headers/modpyb.h` — include `sdcard.h`.
- `ports/atmel-sam3x/modpyb.c` — export `SDCard` in the `machine` module.

---
---

### Task 1: Enable VFS/FATFS/os/io infrastructure

**Files:**
- Modify: `ports/atmel-sam3x/headers/mpconfigport.h`
- Modify: `ports/atmel-sam3x/Makefile:150-167`
- Modify: `ports/atmel-sam3x/main.c:62-74`
- Create: `ports/atmel-sam3x/moduos.c`
- Create: `ports/atmel-sam3x/fatfs_port.c`

**Interfaces:**
- Consumes: `mp_vfs_open` (`extmod/vfs.h`), `mp_vfs_*` fun-objs (`extmod/vfs.h`), `mp_fat_vfs_type` (`extmod/vfs_fat.h`), `mp_module_io` (`py/headers/builtin.h`), `mp_module_uos` (this task).
- Produces: `const mp_obj_module_t mp_module_uos` (registered as `uos` and `os`), `DWORD get_fattime(void)`, `mp_builtin_open` delegating to `mp_vfs_open`. Later tasks rely on: `import os` working and `open()` raising `OSError` when nothing is mounted.

- [ ] **Step 1: Enable VFS and I/O features in `headers/mpconfigport.h`**

Change line 43 from `#define MICROPY_PY_IO               (0)` to `(1)`, and add these defines after line 43:

```c
#define MICROPY_PY_IO               (1)
#define MICROPY_PY_IO_FILEIO        (1)
#define MICROPY_VFS                 (1)
#define MICROPY_VFS_FAT             (1)
#define MICROPY_READER_VFS          (1)
```

- [ ] **Step 2: Register the `uos`/`os`/`io` modules and alias `mp_import_stat` in `headers/mpconfigport.h`**

Replace lines 70-74:

```c
extern const struct _mp_obj_module_t pyb_module;

extern const struct _mp_obj_module_t mp_module_uos;
extern const mp_obj_module_t mp_module_io;

#define mp_import_stat mp_vfs_import_stat

#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_machine), (mp_obj_t)&pyb_module}, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_uos), (mp_obj_t)&mp_module_uos}, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_os), (mp_obj_t)&mp_module_uos}, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_io), (mp_obj_t)&mp_module_io}, \
```

- [ ] **Step 3: Add `lib/oofatfs` and new port sources to `Makefile` `SRC_C`**

Add these entries to `SRC_C` (after line 167, `modrandom.c`):

```makefile
SRC_C = \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	main.c 			\
	due_mphal.c 	\
	led.c 	     	\
	modpyb.c      	\
	time.c 			\
	modpin.c 		\
	pin_named_def.c \
	modadc.c 		\
	daac.c 			\
	modpinmap.c 	\
	modtwi.c 		\
	modpwm.c 		\
	help.c 			\
	modrandom.c 	\
	moduos.c 		\
	fatfs_port.c 	\
	lib/oofatfs/ff.c \
	lib/oofatfs/option/unicode.c \
```

- [ ] **Step 4: Create `ports/atmel-sam3x/moduos.c`**

```c
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include "py/headers/obj.h"
#include "py/headers/objmodule.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

/// \module os - basic "operating system" services
///
/// The `os` module contains functions for filesystem access.

STATIC const mp_map_elem_t os_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_uos) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_chdir), (mp_obj_t)&mp_vfs_chdir_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_getcwd), (mp_obj_t)&mp_vfs_getcwd_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_listdir), (mp_obj_t)&mp_vfs_listdir_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mkdir), (mp_obj_t)&mp_vfs_mkdir_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_remove), (mp_obj_t)&mp_vfs_remove_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_rename),(mp_obj_t)&mp_vfs_rename_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_rmdir), (mp_obj_t)&mp_vfs_rmdir_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_stat), (mp_obj_t)&mp_vfs_stat_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_statvfs), (mp_obj_t)&mp_vfs_statvfs_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_unlink), (mp_obj_t)&mp_vfs_remove_obj },

    { MP_OBJ_NEW_QSTR(MP_QSTR_sep), MP_OBJ_NEW_QSTR(MP_QSTR__slash_) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_mount), (mp_obj_t)&mp_vfs_mount_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_umount), (mp_obj_t)&mp_vfs_umount_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_VfsFat), (mp_obj_t)&mp_fat_vfs_type },
};

STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

const mp_obj_module_t mp_module_uos = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&os_module_globals,
};
```

- [ ] **Step 5: Create `ports/atmel-sam3x/fatfs_port.c`**

```c
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include "lib/oofatfs/ff.h"

// No wall-clock RTC on this port; return a fixed timestamp (2026-01-01 00:00:00).
DWORD get_fattime(void) {
    return ((2026 - 1980) << 25) | (1 << 21) | (1 << 16);
}
```

- [ ] **Step 6: Rework filesystem stubs in `ports/atmel-sam3x/main.c`**

Add `#include "extmod/vfs.h"` after line 19 (`#include "asf.h"`).

Replace lines 62-74 with:

```c
#if !MICROPY_READER_VFS
mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    return NULL;
}
#endif

#if !MICROPY_VFS
mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}
#endif

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_vfs_open(n_args, args, kwargs);
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
```

- [ ] **Step 7: Build**

Run: `make -j8` in `ports/atmel-sam3x/`
Expected: compiles and links; `Creating build/firmware.bin` is the last line; no errors.

- [ ] **Step 8: Flash and verify on hardware**

Run: `make upload port=<your-serial-device>` (bossac via the 1200-baud touch).
Open the REPL (UART or USB CDC) and paste:

```
>>> import os
>>> os.getcwd()
'/'
>>> os.VfsFat
<class 'VfsFat'>
```

Expected: `os.getcwd()` returns `'/'`; `os.VfsFat` prints the class. If no SD card feature exists yet, `os.listdir('/')` may raise `OSError` — that is expected at this stage.

- [ ] **Step 9: Commit**

```bash
git add ports/atmel-sam3x/headers/mpconfigport.h ports/atmel-sam3x/Makefile ports/atmel-sam3x/main.c ports/atmel-sam3x/moduos.c ports/atmel-sam3x/fatfs_port.c
git commit -m "Enable VFS/FATFS and os module on atmel-sam3x port"
```

---
---

### Task 2: Register-level SPI0 master driver

**Files:**
- Create: `ports/atmel-sam3x/spi_master.h`
- Create: `ports/atmel-sam3x/spi_master.c`
- Modify: `ports/atmel-sam3x/Makefile:150-167` (add to `SRC_C`)

**Interfaces:**
- Consumes: ASF `pmc_enable_periph_clk`, `pio_configure`, `sysclk_get_peripheral_hz` (via `asf.h`); CMSIS `SPI0` pointer and `SPI_*` bit macros (`component_spi.h`).
- Produces:
  - `void spi_master_init(void)` — clocks, pins, reset, master mode, enables SPI.
  - `void spi_master_set_frequency(uint32_t freq_hz)` — sets CSR0 SCBR; preserves `spi_master_csat`.
  - `uint8_t spi_master_transfer(uint8_t out)` — blocking single-byte exchange.
  - `void spi_master_transfer_bytes(const uint8_t *out, uint8_t *in, uint32_t len)` — block exchange; `out`/`in` may be `NULL`.
  - `void spi_master_set_csat(bool enable)` — sets/clears `SPI_CSR_CSAAT` for hardware-CS mode (declared now; **implementation is added in Task 7** — do not implement it here).
  Later tasks use `spi_master_transfer`/`spi_master_transfer_bytes` exclusively for all byte I/O.

- [ ] **Step 1: Create `ports/atmel-sam3x/spi_master.h`**

```c
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
```

- [ ] **Step 2: Create `ports/atmel-sam3x/spi_master.c`**

```c
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

void spi_master_set_frequency(uint32_t freq_hz) {
    uint32_t pclk = sysclk_get_peripheral_hz();
    uint32_t scbr = pclk / freq_hz;
    if (scbr < 1) {
        scbr = 1;
    } else if (scbr > 0xff) {
        scbr = 0xff;
    }
    // 8-bit transfers, SPI mode 0, baud-rate divisor for CS0
    uint32_t csr = SPI_CSR_BITS_8_BIT | SPI_CSR_SCBR(scbr);
    SPI0->SPI_CSR[0] = csr;
}

// NOTE: spi_master_set_csat() is declared in the header but its body is added
// in Task 7 together with hardware-CS support in the SD layer.

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
```

- [ ] **Step 3: Add `spi_master.c` to the Makefile**

Append `spi_master.c \` to `SRC_C` (after `modrandom.c`, before `moduos.c`).

- [ ] **Step 4: Build**

Run: `make -j8` in `ports/atmel-sam3x/`
Expected: compiles and links cleanly (no warnings about the new file). `Creating build/firmware.bin` is the last line.

- [ ] **Step 5: Commit**

```bash
git add ports/atmel-sam3x/spi_master.h ports/atmel-sam3x/spi_master.c ports/atmel-sam3x/Makefile
git commit -m "Add register-level SPI0 master driver for atmel-sam3x"
```

---
---

### Task 3: SD protocol layer — init and single-block read/write

**Files:**
- Create: `ports/atmel-sam3x/sdcard_spi.h`
- Create: `ports/atmel-sam3x/sdcard_spi.c`
- Modify: `ports/atmel-sam3x/Makefile:150-167` (add to `SRC_C`)

**Interfaces:**
- Consumes: `spi_master_init/set_frequency/transfer/transfer_bytes` (Task 2), `ini_pin`/`set_pin` (`headers/modpin.h`), `time_millis` (`headers/time.h`), `BP_IOCTL_*` (`extmod/vfs.h`), `delay_ms` (`asf.h`).
- Produces:
  - `#define SDCARD_BLOCK_SIZE 512`
  - `typedef struct _sdcard_config_t { uint8_t cs_pin; uint32_t spi_freq; bool use_hw_cs; } sdcard_config_t;`
  - `bool sdcard_init(sdcard_config_t *config)` — powers up, CMD0/CMD8/ACMD41/CMD58/CMD16, raises clock to `config->spi_freq`.
  - `bool sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks)` — CMD17 (1 block) or CMD18+CMD12 (multi).
  - `bool sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks)` — CMD24 (1 block) or CMD25 (multi).
  - `uint32_t sdcard_ioctl(uint32_t cmd, uint32_t arg)` — `BP_IOCTL_INIT/DEINIT/SYNC/SEC_COUNT/SEC_SIZE`.
  - `bool sdcard_is_present(void)` — CMD13 status read; `false` if no response.
  Task 4 adds multi-block body + CSD parsing; Task 5 consumes all of the above.

- [ ] **Step 1: Create `ports/atmel-sam3x/sdcard_spi.h`**

```c
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
```

- [ ] **Step 2: Create `ports/atmel-sam3x/sdcard_spi.c`**

```c
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
```

- [ ] **Step 3: Add `sdcard_spi.c` to the Makefile**

Append `sdcard_spi.c \` to `SRC_C` (after `spi_master.c`).

- [ ] **Step 4: Build**

Run: `make -j8` in `ports/atmel-sam3x/`
Expected: compiles and links cleanly. `Creating build/firmware.bin` is the last line.

- [ ] **Step 5: Commit**

```bash
git add ports/atmel-sam3x/sdcard_spi.h ports/atmel-sam3x/sdcard_spi.c ports/atmel-sam3x/Makefile
git commit -m "Add SD protocol layer: init and single-block I/O for atmel-sam3x"
```

---
---

### Task 4: SD protocol layer — multi-block I/O and sector count

**Files:**
- Modify: `ports/atmel-sam3x/sdcard_spi.c` (the two `return false;` branches for CMD18/CMD25, and `BP_IOCTL_SEC_COUNT`)

**Interfaces:**
- Consumes: the statics and helpers defined in Task 3 (`sdcard_cmd`, `sdcard_wait_data_token`, `sdcard_wait_not_busy`, `sdcard_block_addressing`).
- Produces: full multi-block read (`CMD18` + `CMD12` stop) and write (`CMD25` + stop token `0xfd`); `BP_IOCTL_SEC_COUNT` parsing CSD v1/v2.

- [ ] **Step 1: Implement multi-block read in `sdcard_read_blocks`**

Replace this block in `ports/atmel-sam3x/sdcard_spi.c`:

```c
    } else {
        // CMD18: multiple block read (multi-block body added in Task 4)
        sdcard_cs_high();
        return false;
    }
```

with:

```c
    } else {
        // CMD18: multiple block read
        if (sdcard_cmd(18, addr, false) != 0x00) {
            sdcard_cs_high();
            return false;
        }
        for (uint32_t i = 0; i < num_blocks; i++) {
            if (sdcard_wait_data_token(0xfe) != 0xfe) {
                sdcard_cmd(12, 0, false); // stop transmission
                sdcard_cs_high();
                return false;
            }
            spi_master_transfer_bytes(NULL, dest + i * SDCARD_BLOCK_SIZE, SDCARD_BLOCK_SIZE);
            spi_master_transfer_bytes(NULL, NULL, 2); // CRC16
        }
        sdcard_cmd(12, 0, false); // stop transmission
    }
```

- [ ] **Step 2: Implement multi-block write in `sdcard_write_blocks`**

Replace this block:

```c
    } else {
        // CMD25: multiple block write (multi-block body added in Task 4)
        sdcard_cs_high();
        return false;
    }
```

with:

```c
    } else {
        // CMD25: multiple block write
        if (sdcard_cmd(25, addr, false) != 0x00) {
            sdcard_cs_high();
            return false;
        }
        for (uint32_t i = 0; i < num_blocks; i++) {
            spi_master_transfer(0xfc); // multi-block start token
            spi_master_transfer_bytes(src + i * SDCARD_BLOCK_SIZE, NULL, SDCARD_BLOCK_SIZE);
            uint16_t crc = sdcard_crc16(src + i * SDCARD_BLOCK_SIZE, SDCARD_BLOCK_SIZE);
            spi_master_transfer((uint8_t)(crc >> 8));
            spi_master_transfer((uint8_t)(crc & 0xff));

            uint8_t resp = spi_master_transfer(0xff);
            if ((resp & 0x1f) != 0x05) {
                sdcard_cs_high();
                return false;
            }
            if (!sdcard_wait_not_busy()) {
                sdcard_cs_high();
                return false;
            }
        }
        spi_master_transfer(0xfd); // stop token
        if (!sdcard_wait_not_busy()) {
            sdcard_cs_high();
            return false;
        }
    }
```

- [ ] **Step 3: Add CSD block-count parsing**

Add this function after `sdcard_wait_not_busy()` (before the `// ---- public API ----` comment):

```c
// Returns the number of 512-byte blocks from a 16-byte CSD register
// (read via CMD9). Handles both CSD v1 and v2.
static uint32_t sdcard_csd_block_count(const uint8_t *csd) {
    uint8_t csd_structure = (csd[0] >> 6) & 0x03;
    if (csd_structure == 0x01) {
        // CSD v2 (SDHC/SDXC): C_SIZE = bits [69:48]
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3f) << 16)
                        | ((uint32_t)csd[8] << 8)
                        | csd[9];
        return (c_size + 1) * 1024;
    }
    // CSD v1 (SDSC): READ_BL_LEN = bits [83:80], C_SIZE = bits [73:62],
    // C_SIZE_MULT = bits [49:47]
    uint32_t c_size = ((uint32_t)(csd[6] & 0x03) << 10)
                    | ((uint32_t)csd[7] << 2)
                    | ((csd[8] >> 6) & 0x03);
    uint32_t c_size_mult = ((uint32_t)(csd[9] & 0x03) << 1)
                         | ((csd[10] >> 7) & 0x01);
    uint32_t read_bl_len = csd[5] & 0x0f;
    uint32_t block_nr = (c_size + 1) * (1 << (c_size_mult + 2));
    if (read_bl_len > 9) {
        block_nr <<= (read_bl_len - 9);
    }
    return block_nr;
}
```

- [ ] **Step 4: Wire `BP_IOCTL_SEC_COUNT` to the CSD parse**

Replace this case in `sdcard_ioctl`:

```c
        case BP_IOCTL_SEC_COUNT:
            return 0; // implemented in Task 4 via CSD parse
```

with:

```c
        case BP_IOCTL_SEC_COUNT:
        {
            // read CSD (CMD9) and return the sector count
            sdcard_cs_low();
            uint8_t r1 = sdcard_cmd(9, 0, false);
            uint32_t count = 0;
            if (r1 == 0x00 && sdcard_wait_data_token(0xfe) == 0xfe) {
                uint8_t csd[16];
                spi_master_transfer_bytes(NULL, csd, 16);
                spi_master_transfer_bytes(NULL, NULL, 2); // CRC16
                count = sdcard_csd_block_count(csd);
            }
            sdcard_cs_high();
            return count;
        }
```

- [ ] **Step 5: Build**

Run: `make -j8` in `ports/atmel-sam3x/`
Expected: compiles and links cleanly.

- [ ] **Step 6: Commit**

```bash
git add ports/atmel-sam3x/sdcard_spi.c
git commit -m "Add multi-block SD I/O and CSD sector count for atmel-sam3x"
```

---
---

### Task 5: `machine.SDCard` binding

**Files:**
- Create: `ports/atmel-sam3x/headers/sdcard.h`
- Create: `ports/atmel-sam3x/modsdcard.c`
- Modify: `ports/atmel-sam3x/headers/modpyb.h` (include `sdcard.h`)
- Modify: `ports/atmel-sam3x/modpyb.c:81-99` (export `SDCard` in `machine`)
- Modify: `ports/atmel-sam3x/Makefile:150-167` (add to `SRC_C`)

**Interfaces:**
- Consumes: `pin_find` (`headers/pin_named_def.h`), `pyb_pin_obj` (`headers/modpinmap.h`), `sdcard_*` (Task 3/4), `BP_IOCTL_*` (`extmod/vfs.h`).
- Produces: `const mp_obj_type_t pyb_sdcard_type` (name `SDCard`) with `make_new(cs_pin[, freq[, use_hw_cs]])` and methods `readblocks`, `writeblocks`, `ioctl`, `present`. `modpyb.c` exports it as `machine.SDCard`. MicroPython usage: `SDCard(Pin(77))` then `os.VfsFat(sd)`/`os.mount(...)`.

- [ ] **Step 1: Create `ports/atmel-sam3x/headers/sdcard.h`**

```c
#ifndef SDCARD_H
#define SDCARD_H

extern const mp_obj_type_t pyb_sdcard_type;

#endif
```

- [ ] **Step 2: Create `ports/atmel-sam3x/modsdcard.c`**

```c
/*
 * machine.SDCard for the atmel-sam3x (Arduino Due) port.
 *
 * Exposes the SPI SD card as a MicroPython block device so that it can be
 * wrapped with os.VfsFat and mounted:
 *
 *     from machine import SDCard, Pin
 *     import os
 *     sd = SDCard(Pin(77))            # PA28 / NPCS0
 *     os.mount(os.VfsFat(sd), "/sd")
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

#include <stdint.h>

#include "py/headers/obj.h"
#include "py/headers/runtime.h"
#include "modpinmap.h"
#include "pin_named_def.h"
#include "extmod/vfs.h"
#include "sdcard_spi.h"
#include "sdcard.h"

typedef struct _pyb_sdcard_obj_t {
    mp_obj_base_t base;
} pyb_sdcard_obj_t;

// Single-card driver: sdcard_spi.c holds the one instance's configuration.
STATIC const pyb_sdcard_obj_t pyb_sdcard_obj = { { &pyb_sdcard_type } };

STATIC mp_obj_t pyb_sdcard_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 3, false);

    const pyb_pin_obj *cs_pin = pin_find(args[0]);
    uint32_t freq = (n_args >= 2) ? mp_obj_get_int(args[1]) : 25000000;
    bool use_hw_cs = (n_args >= 3) ? mp_obj_is_true(args[2]) : false;

    sdcard_config_t config;
    config.cs_pin = cs_pin->board_pin;
    config.spi_freq = freq;
    config.use_hw_cs = use_hw_cs;

    if (!sdcard_init(&config)) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "SDCard init failed"));
    }
    return (mp_obj_t)&pyb_sdcard_obj;
}

STATIC mp_obj_t pyb_sdcard_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);
    bool ok = sdcard_read_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ok);
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_readblocks_obj, pyb_sdcard_readblocks);

STATIC mp_obj_t pyb_sdcard_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);
    bool ok = sdcard_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ok);
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_writeblocks_obj, pyb_sdcard_writeblocks);

STATIC mp_obj_t pyb_sdcard_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    mp_int_t arg = mp_obj_get_int(arg_in);
    return MP_OBJ_NEW_SMALL_INT(sdcard_ioctl(cmd, arg));
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_ioctl_obj, pyb_sdcard_ioctl);

STATIC mp_obj_t pyb_sdcard_present(mp_obj_t self) {
    return mp_obj_new_bool(sdcard_is_present());
}
MP_DEFINE_CONST_FUN_OBJ_1(pyb_sdcard_present_obj, pyb_sdcard_present);

STATIC const mp_map_elem_t pyb_sdcard_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_present), (mp_obj_t)&pyb_sdcard_present_obj },
    // block device protocol
    { MP_OBJ_NEW_QSTR(MP_QSTR_readblocks), (mp_obj_t)&pyb_sdcard_readblocks_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_writeblocks), (mp_obj_t)&pyb_sdcard_writeblocks_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ioctl), (mp_obj_t)&pyb_sdcard_ioctl_obj },
};

STATIC MP_DEFINE_CONST_DICT(pyb_sdcard_locals_dict, pyb_sdcard_locals_dict_table);

const mp_obj_type_t pyb_sdcard_type = {
    { &mp_type_type },
    .name = MP_QSTR_SDCard,
    .make_new = pyb_sdcard_make_new,
    .locals_dict = (mp_obj_t)&pyb_sdcard_locals_dict,
};
```

- [ ] **Step 3: Include `sdcard.h` from `headers/modpyb.h`**

Add `#include "sdcard.h"` after line 11 (`#include "modrandom.h"`).

- [ ] **Step 4: Export `SDCard` in `modpyb.c`**

Add this entry after line 92 (`{MP_OBJ_NEW_QSTR(MP_QSTR_PWM), (mp_obj_t)&pwm_type},`):

```c
	{MP_OBJ_NEW_QSTR(MP_QSTR_SDCard), (mp_obj_t)&pyb_sdcard_type},
```

- [ ] **Step 5: Add `modsdcard.c` to the Makefile**

Append `modsdcard.c \` to `SRC_C` (after `sdcard_spi.c`).

- [ ] **Step 6: Build**

Run: `make -j8` in `ports/atmel-sam3x/`
Expected: compiles and links cleanly.

- [ ] **Step 7: Flash and verify raw block I/O on hardware**

Run: `make upload port=<your-serial-device>`. Insert a FAT16/FAT32 formatted microSD (or a plain SD) wired to PA25/26/27 with CS on PA28. Paste into the REPL:

```
>>> from machine import SDCard, Pin
>>> sd = SDCard(Pin(77))
>>> sd.present()
True
>>> buf = bytearray(512)
>>> sd.readblocks(0, buf)
True
>>> buf[510], buf[511]
(85, 170)
```

Expected: `present()` → `True`; `readblocks(0, buf)` → `True`; `buf[510], buf[511]` → `(85, 170)` (the `0x55 0xaa` boot signature on a formatted card). If the card is blank, the last two bytes may differ but `readblocks` must still return `True`.

- [ ] **Step 8: Commit**

```bash
git add ports/atmel-sam3x/headers/sdcard.h ports/atmel-sam3x/modsdcard.c ports/atmel-sam3x/headers/modpyb.h ports/atmel-sam3x/modpyb.c ports/atmel-sam3x/Makefile
git commit -m "Add machine.SDCard block device for atmel-sam3x"
```

---
---

### Task 6: End-to-end FAT filesystem verification

**Files:**
- Modify: none (verification only)

**Interfaces:**
- Consumes: everything from Tasks 1-5.

- [ ] **Step 1: Flash the firmware**

Run: `make upload port=<your-serial-device>` in `ports/atmel-sam3x/`

- [ ] **Step 2: Run the end-to-end mount + file test in the REPL**

With a formatted SD card inserted, paste:

```
>>> from machine import SDCard, Pin
>>> import os
>>> sd = SDCard(Pin(77))
>>> os.mount(os.VfsFat(sd), "/sd")
>>> os.listdir("/sd")
[]
>>> f = open("/sd/hello.txt", "w")
>>> f.write("hello sd card\n")
13
>>> f.close()
>>> f = open("/sd/hello.txt", "r")
>>> f.read()
'hello sd card\n'
>>> f.close()
>>> os.listdir("/sd")
['HELLO.TXT']
>>> os.unmount("/sd")
```

Expected: `os.mount` succeeds (no exception), the file round-trips through `open()`, and `os.listdir` shows `HELLO.TXT` (FAT uppercases names; `_USE_LFN` is disabled so long filenames are truncated/uppercased — a short name like `hello.txt` works).

If `os.mount` raises `OSError` because the card is blank, format it first with:

```
>>> os.VfsFat.mkfs(sd)
>>> os.mount(os.VfsFat(sd), "/sd")
```

- [ ] **Step 3: Verify multi-block I/O via a larger file**

```
>>> f = open("/sd/big.bin", "wb")
>>> f.write(bytes(range(256)) * 40)   # 10240 bytes, crosses multiple 512-byte sectors
10240
>>> f.close()
>>> f = open("/sd/big.bin", "rb")
>>> d = f.read()
>>> len(d)
10240
>>> d[:256] == bytes(range(256))
True
>>> d[256:512] == bytes(range(256))
True
>>> f.close()
>>> os.remove("/sd/big.bin")
```

Expected: writes and reads 10240 bytes correctly; data matches.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "Verify FAT file storage on SPI SD card for atmel-sam3x"
```

(If Step 2/3 revealed a bug, fix it in a normal task-style commit rather than committing test output; see Task 7's notes for the hardware-CS path.)

---
---

### Task 7: Hardware NPCS0 chip-select support (CSAAT/LASTXFER)

**Files:**
- Modify: `ports/atmel-sam3x/spi_master.c` (implement `spi_master_set_csat`)
- Modify: `ports/atmel-sam3x/sdcard_spi.c` (CS helpers honor `use_hw_cs`)

**Interfaces:**
- Consumes: `SPI_CSR_CSAAT` and `SPI_TDR_LASTXFER` (CMSIS), `sdcard_config.use_hw_cs`.
- Produces: when `SDCard(pin, freq, True)` is used, the SPI peripheral drives NPCS0 automatically; CS stays low across a whole transaction and is released by `LASTXFER`.

- [ ] **Step 1: Implement `spi_master_set_csat` and preserve it across frequency changes**

In `ports/atmel-sam3x/spi_master.c`, add a static flag and make `spi_master_set_frequency` preserve it. Replace the frequency function body:

```c
void spi_master_set_frequency(uint32_t freq_hz) {
    uint32_t pclk = sysclk_get_peripheral_hz();
    uint32_t scbr = pclk / freq_hz;
    if (scbr < 1) {
        scbr = 1;
    } else if (scbr > 0xff) {
        scbr = 0xff;
    }
    // 8-bit transfers, SPI mode 0, baud-rate divisor for CS0
    uint32_t csr = SPI_CSR_BITS_8_BIT | SPI_CSR_SCBR(scbr);
    SPI0->SPI_CSR[0] = csr;
}
```

with:

```c
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
```

Also add `#include <stdbool.h>` at the top of `spi_master.c`.

- [ ] **Step 2: Make the SD layer's CS helpers honor `use_hw_cs`**

In `ports/atmel-sam3x/sdcard_spi.c`, replace the `// ---- chip select ----` section (including the `sdcard_cs_low`/`sdcard_cs_high` functions) with:

```c
// ---- chip select ----
// GPIO mode drives the CS pin directly. Hardware-CS mode relies on
// SPI_CSR_CSAAT so the peripheral asserts NPCS0 for the first byte of a
// transaction; the LASTXFER byte (written in sdcard_cs_high) releases it.

static void sdcard_cs_low(void) {
    if (!sdcard_config.use_hw_cs) {
        set_pin(sdcard_config.cs_pin, 0);
    }
}

static void sdcard_cs_high(void) {
    if (sdcard_config.use_hw_cs) {
        // deassert NPCS0 after this byte
        while (!(SPI0->SPI_SR & SPI_SR_TDRE)) {
        }
        SPI0->SPI_TDR = SPI_TDR_LASTXFER;
        while (!(SPI0->SPI_SR & SPI_SR_RDRF)) {
        }
    } else {
        set_pin(sdcard_config.cs_pin, 1);
    }
}
```

- [ ] **Step 3: Enable CSAAT in `sdcard_init` when `use_hw_cs` is set**

In `ports/atmel-sam3x/sdcard_spi.c`, right after the `// SPI at init clock speed` call to `spi_master_set_frequency(SDCARD_INIT_CLOCK_HZ);`, add:

```c
    if (sdcard_config.use_hw_cs) {
        // configure PA28 as NPCS0 (peripheral A) and use hardware CS
        pio_configure(PIOA, PIO_PERIPH_A, PIO_PA28A_SPI0_NPCS0, PIO_DEFAULT);
        spi_master_set_csat(true);
    }
```

Note: the CS pin is still configured as a GPIO output by the existing `ini_pin(sdcard_config.cs_pin, OUTPUT)` call at the top of `sdcard_init`; that call is harmless (the pin is simply re-typed to NPCS0 here) but if you prefer, only issue it when `!use_hw_cs`. The `SPI0`/`SPI_SR_TDRE`/`SPI_TDR_LASTXFER` identifiers are available because `sdcard_spi.c` includes `asf.h`.

- [ ] **Step 4: Build**

Run: `make -j8` in `ports/atmel-sam3x/`
Expected: compiles and links cleanly.

- [ ] **Step 5: Verify hardware CS mode on hardware**

Flash and paste into the REPL (SD on PA25/26/27, CS wired to PA28/NPCS0):

```
>>> from machine import SDCard, Pin
>>> sd = SDCard(Pin(77), 25000000, True)
>>> sd.present()
True
>>> buf = bytearray(512)
>>> sd.readblocks(0, buf)
True
>>> import os
>>> os.mount(os.VfsFat(sd), "/sd")
>>> os.listdir("/sd")
[]
```

Expected: same results as GPIO-CS mode (`True` / mount success).

- [ ] **Step 6: Commit**

```bash
git add ports/atmel-sam3x/spi_master.c ports/atmel-sam3x/sdcard_spi.c
git commit -m "Add hardware NPCS0 chip-select mode for atmel-sam3x SD card"
```

---
---

## Self-Review

- **Spec coverage:** file-storage via `open()` (Tasks 1+6); SPI0 + PA25/26/27/28 (Tasks 2-3); configurable CS (Tasks 3, 5, 7); polling card detect (`sdcard_is_present` → `present()`, Task 3); 400 kHz init / up-to-25 MHz data (Task 3); block read/write incl. multi-block (Tasks 3-4); ioctl SEC_COUNT/SEC_SIZE (Tasks 3-4); `machine.SDCard` (Task 5); VFS mount at `/sd` (Task 6). CRC7 commands + CRC16 data implemented (Task 3). Removed from spec by documented deviations: CMD2/3/7 (not used in SPI mode), the non-existent ASF SPI driver, and `machine.SPI`-based constructor.
- **Placeholder scan:** no TBD/TODO; every code step shows full code; the two "added in Task 4" markers in Task 3 are resolved by Task 4 steps, and the "implementation added in Task 7" marker for `spi_master_set_csat` is resolved by Task 7 Step 1.
- **Type consistency:** `sdcard_config_t`, `sdcard_init/read_blocks/write_blocks/ioctl/is_present`, `pyb_sdcard_type`, `spi_master_*` signatures are identical across all tasks that reference them. `BP_IOCTL_*` matches `extmod/vfs.h`. `SDCard_BLOCK_SIZE` is 512 everywhere. `use_hw_cs` semantics (GPIO vs CSAAT/LASTXFER) are consistent between Task 3 comments, Task 5 constructor, and Task 7.

## Execution Notes (added during implementation)

- **Task 1:** the port also needed `$(CFLAGS_MOD)` folded into `CFLAGS` (picks up `-DFFCONF_H="lib/oofatfs/ffconf.h"` from `py/py.mk`), `lib/timeutils/timeutils.c` in `SRC_C` (`extmod/vfs_fat.c` calls `timeutils_seconds_since_2000`), `#define MICROPY_FATFS_RPATH (2)` (`f_chdir`/`f_getcwd`), `#define mp_type_fileio fatfs_type_fileio` (`py/modio.c` needs it), and a port-level `mp_sys_stdout_obj`/`mp_sys_stdout_print` in `main.c` (`MP_PYTHON_PRINTER` needs them with `MICROPY_PY_SYS=0`). All mirror the stm32 port.
- **Task 3 CRC7 fix:** the original CRC7 in this plan (shift-with-poly-0x09 form) was WRONG — it produced 0x37/0x40 instead of the required 0x4A/0x43 for CMD0/CMD8 (verified against the polynomial definition and known-good reference bytes 0x95/0x87). The code above is the corrected, verified bit-feeding form (`M(x)*x^7 mod G(x)`, G=0x89). CMD0 is also now retried up to 5× and CMD8's echo is validated (echo[2]==0x01, echo[3]==0xaa).
