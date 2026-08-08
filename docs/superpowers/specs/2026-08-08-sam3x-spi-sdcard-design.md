# SPI SD Card Support for Atmel SAM3X (Arduino Due)

## Overview

Add support for external SD card via SPI0 on the Atmel SAM3X/Arduino Due port, enabling file storage (read/write files from Python) using the MicroPython FAT filesystem (oofatfs).

## Requirements

- **Target**: File storage via Python's `open()` and standard filesystem operations
- **Interface**: External SD card connected to SPI0
- **SPI Pins**: PA25 (MISO), PA26 (MOSI), PA27 (SCK), PA28 (NPCS0)
- **Chip Select**: Configurable - hardware NPCS0 or software GPIO
- **Card Detection**: Polling only (no dedicated card detect pin)
- **Speed**: SPI mode (adequate for file storage; ~400kHz init, up to 25MHz data)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    MicroPython VM                            │
├─────────────────────────────────────────────────────────────┤
│  machine.SDCard (modsdcard.c)                                │
│  - readblocks(), writeblocks(), ioctl()                     │
└──────────────────────┬──────────────────────────────────────┘
                       │ Block Device Protocol
┌──────────────────────▼──────────────────────────────────────┐
│  SD/MMC SPI Protocol Layer (sdcard_spi.c)                   │
│  - sdcard_init(), sdcard_read_blocks(), sdcard_write_blocks()│
│  - sdcard_ioctl(), sdcard_is_present()                      │
│  - CMD/response handling, CRC7/CRC16                        │
└──────────────────────┬──────────────────────────────────────┘
                       │ SPI Transfers
┌──────────────────────▼──────────────────────────────────────┐
│  ASF SPI Driver (existing: ASF/sam/drivers/spi/spi.c)       │
│  - spi_master_init(), spi_write(), spi_read()               │
└──────────────────────┬──────────────────────────────────────┘
                       │ Hardware SPI0
┌──────────────────────▼──────────────────────────────────────┐
│  External SD Card (via SPI0: PA25/26/27/28)                 │
└─────────────────────────────────────────────────────────────┘
```

## Components

### 1. SD/MMC SPI Protocol Layer (`sdcard_spi.c` / `sdcard_spi.h`)

Implements the SD card protocol over SPI:

**Initialization Sequence:**
1. 74+ clock cycles with CS high (power-up)
2. CMD0 (GO_IDLE_STATE) - reset card
3. CMD8 (SEND_IF_COND) - check voltage range (2.7-3.6V)
4. ACMD41 (SD_SEND_OP_COND) - initialize card, check HC/XC
5. CMD2 (ALL_SEND_CID) - get CID
6. CMD3 (SEND_RELATIVE_ADDR) - get RCA
7. CMD7 (SELECT_CARD) - select card
8. ACMD6 (SET_BUS_WIDTH) - set 4-bit (optional, not used in SPI)
9. CMD16 (SET_BLOCKLEN) - set 512-byte blocks (for SDSC)

**Commands Supported:**
- CMD0, CMD8, CMD9 (SEND_CSD), CMD10 (SEND_CID), CMD12 (STOP_TRANSMISSION)
- CMD16 (SET_BLOCKLEN), CMD17 (READ_SINGLE_BLOCK), CMD18 (READ_MULTIPLE_BLOCK)
- CMD24 (WRITE_BLOCK), CMD25 (WRITE_MULTIPLE_BLOCK)
- CMD55 (APP_CMD) + ACMD41, ACMD6, ACMD13 (SD_STATUS), ACMD23 (SET_WR_BLK_ERASE_COUNT)

**Block Operations:**
- Single block read/write (CMD17/CMD24)
- Multiple block read/write (CMD18/CMD25 + CMD12)
- CRC7 for commands, CRC16 for data

**Error Handling:**
- Response timeout detection
- CRC error detection
- Data token validation (0xFE for read, 0xFC/0xFD for write)

### 2. Block Device Interface (in `sdcard_spi.c`)

```c
// Initialize SD card and SPI
bool sdcard_init(spi_master_config_t *config, Pin cs_pin, bool use_hw_cs);

// Read 512-byte blocks
mp_uint_t sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks);

// Write 512-byte blocks
mp_uint_t sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks);

// Block device ioctl
mp_uint_t sdcard_ioctl(mp_uint_t cmd, mp_uint_t arg);

// Check card presence (polling)
bool sdcard_is_present(void);
```

**ioctl Commands:**
- `BP_IOCTL_INIT` - Initialize card
- `BP_IOCTL_DEINIT` - Deinitialize card
- `BP_IOCTL_SYNC` - Flush writes (no-op for SD)
- `BP_IOCTL_SEC_COUNT` - Return sector count
- `BP_IOCTL_SEC_SIZE` - Return sector size (512)

### 3. MicroPython Binding (`modsdcard.c`)

Exposes `machine.SDCard` class implementing the block protocol:

```python
from machine import SDCard, SPI, Pin

# Hardware CS (NPCS0)
sd = SDCard(SPI(0), Pin.cpu.PA28)

# Software CS (any GPIO)
sd = SDCard(SPI(0), Pin.cpu.PA25)

# Mount filesystem
import os
os.mount(sd, "/sd")
```

**Methods:**
- `readblocks(block_num, buf)` - Read blocks into buffer
- `writeblocks(block_num, buf)` - Write blocks from buffer
- `ioctl(cmd, arg)` - Block device control
- `present()` - Check if card is present (polling)

### 4. VFS Integration (`main.c`)

- Initialize SPI0 and SD card on boot (optional, can be lazy)
- Mount FAT filesystem at `/sd` when SDCard object created
- Handle mount/unmount via Python API

## Files

### New Files
| File | Purpose |
|------|---------|
| `ports/atmel-sam3x/sdcard_spi.c` | SD/MMC SPI protocol implementation |
| `ports/atmel-sam3x/sdcard_spi.h` | Public API header |
| `ports/atmel-sam3x/modsdcard.c` | MicroPython SDCard class binding |

### Modified Files
| File | Changes |
|------|---------|
| `ports/atmel-sam3x/Makefile` | Add `sdcard_spi.c`, `modsdcard.c` to `SRC_C` |
| `ports/atmel-sam3x/main.c` | Initialize SPI0, optional early SD init |
| `ports/atmel-sam3x/modpyb.c` | Export `SDCard` type in `machine` module |

## Configuration

### Makefile Changes
```makefile
SRC_C = \
    ...
    sdcard_spi.c \
    modsdcard.c \
    ...

# Ensure ASF SPI driver is included (already present)
SRC_ASF += \
    ASF/sam/drivers/spi/spi.c \
    ...
```

### Pin Definitions (add to board header or modsdcard.c)
```c
// Default SPI0 pins for Arduino Due
#define SD_SPI_SPI        SPI0
#define SD_SPI_ID         ID_SPI0
#define SD_SPI_MISO       PIO_PA25_IDX
#define SD_SPI_MOSI       PIO_PA26_IDX
#define SD_SPI_SPCK       PIO_PA27_IDX
#define SD_SPI_NPCS0      PIO_PA28_IDX
#define SD_SPI_MISO_FLAGS (PIO_PERIPH_A | PIO_DEFAULT)
#define SD_SPI_MOSI_FLAGS (PIO_PERIPH_A | PIO_DEFAULT)
#define SD_SPI_SPCK_FLAGS (PIO_PERIPH_A | PIO_DEFAULT)
#define SD_SPI_NPCS0_FLAGS (PIO_PERIPH_A | PIO_DEFAULT)
```

## Testing Plan

1. **Unit Tests** (Python):
   - Initialize SDCard with hardware CS
   - Initialize SDCard with software CS
   - Read/write single block
   - Read/write multiple blocks
   - Mount/unmount FAT filesystem
   - Create/read/write/delete files

2. **Integration Tests**:
   - Stress test with large file operations
   - Power cycle resilience
   - Card removal/insertion handling (polling)

3. **Performance**:
   - Measure read/write throughput
   - Verify SPI clock speeds

## Dependencies

- ASF SPI driver (already in codebase)
- oofatfs (lib/oofatfs/) - already included
- extmod/vfs_fat - already included

## Non-Goals

- HSMCI (native 4-bit SDIO) support - separate feature
- SDIO mode (requires different hardware)
- DMA support for SPI (not available on SAM3X SPI)
- Card detect interrupt (polling only)

## Future Extensions

- Add HSMCI support for built-in microSD slot
- DMA support via PDC (if available)
- SD card benchmarking module