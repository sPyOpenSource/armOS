# Select the board to build for: if not given on the command line,
# then default to teensy
# to set frdmk64f
#make BOARD=frdmk64f
BOARD ?= frdmk64f
ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

# If the build directory is not given, make it reflect the board name.
BUILD ?= build-$(BOARD)

include ../../py/mkenv.mk
-include mpconfigport.mk
include boards/$(BOARD)/mpconfigboard.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h $(BUILD)/pins_qstr.h

# include py core make definitions
include ../../py/py.mk

# If you set USE_ARDUINO_TOOLCHAIN=1 then this makefile will attempt to use
# the toolchain that comes with Teensyduino
USE_ARDUINO_TOOLCHAIN = 1
ARDUINO = /Users/xuyi/Library/Arduino15/packages/arduino
ifeq ($(USE_ARDUINO_TOOLCHAIN),1)
ifeq ($(ARDUINO),)
$(error USE_ARDUINO_TOOLCHAIN requires that ARDUINO be set)
endif
endif

ifeq ($(USE_ARDUINO_TOOLCHAIN),1)
$(info Using ARDUINO toolchain)
CROSS_COMPILE = $(ARDUINO)/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-
else
$(info Using toolchain from PATH)
CROSS_COMPILE = arm-none-eabi-
endif

#CFLAGS_MPCONFIG = -DF_CPU=96000000 -DUSB_SERIAL -D__MK20DX256__ mpconfigboard.mk
CFLAGS_CORTEX_M4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -msoft-float -mfloat-abi=soft -fsingle-precision-constant -Wdouble-promotion $(CFLAGS_MPCONFIG)

INC =  -I.
INC += -I..
INC += -I../..
INC += -I../stm32
INC += -I../stm32/headers
INC += -I../../lib/mp-readline
INC += -I$(BUILD)
INC += -Icore

CFLAGS = $(INC) -Wall -Wpointer-arith -ansi -std=gnu99 -nostdlib $(CFLAGS_CORTEX_M4)
CFLAGS += -Iboards/$(BOARD)
LDFLAGS = -nostdlib -T $(LD_FILE) $(LDFLAGS_MPCONFIG)
ifeq ($(USE_ARDUINO_TOOLCHAIN),1)

LIBGCC_FILE_NAME = $(ARDUINO)/hardware/tools/arm-none-eabi/lib/gcc/arm-none-eabi/4.7.2/thumb2/libgcc.a
LIBM_FILE_NAME   = $(ARDUINO)/hardware/tools/arm-none-eabi/arm-none-eabi/lib/thumb2/libm.a
LIBC_FILE_NAME   = $(ARDUINO)/hardware/tools/arm-none-eabi/arm-none-eabi/lib/thumb2/libc.a

else

LIBGCC_FILE_NAME = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
LIBM_FILE_NAME   = $(shell $(CC) $(CFLAGS) -print-file-name=libm.a)
LIBC_FILE_NAME   = $(shell $(CC) $(CFLAGS) -print-file-name=libc.a)

endif

#$(info %%%%% LIBGCC_FILE_NAME = $(LIBGCC_FILE_NAME))
#$(info %%%%% LIBM_FILE_NAME = $(LIBM_FILE_NAME))
#$(info %%%%% LIBC_FILE_NAME = $(LIBC_FILE_NAME))

#$(info %%%%% dirname LIBGCC_FILE_NAME = $(dir $(LIBGCC_FILE_NAME)))
#$(info %%%%% dirname LIBM_FILE_NAME = $(dir $(LIBM_FILE_NAME)))
#$(info %%%%% dirname LIBC_FILE_NAME = $(dir $(LIBC_FILE_NAME)))

LIBS  = -L $(dir $(LIBM_FILE_NAME)) -lm
LIBS += -L $(dir $(LIBC_FILE_NAME)) -lc
LIBS += -L $(dir $(LIBGCC_FILE_NAME)) -lgcc

#Debugging/Optimization
ifdef DEBUG
CFLAGS += -Og -ggdb
else
CFLAGS += -Os #-DNDEBUG
endif
CFLAGS += -fdata-sections -ffunction-sections
#LDFLAGS += -Wl,--gc-sections #passing to linker directly not through cc

SRC_C = \
	help.c \
	import.c \
	lcd.c \
	led.c \
	lexermemzip.c \
	memzip.c \
	modpyb.c \
	reg.c \
	uart.c \
	usb.c \
	$(wildcard boards/$(BOARD)/*.c)

SRC_S = $(SRC_MPCONFIG_S)

STM_SRC_C = $(addprefix stmhal/,\
	gccollect.c \
	input.c \
	irq.c \
	pin.c \
	pin_named_pins.c \
	printf.c \
	pyexec.c \
	pybstdio.c \
	)

STM_SRC_S = $(addprefix stmhal/,\
	gchelper.s \
	)

LIB_SRC_C = $(addprefix lib/,\
	libc/string0.c \
	mp-readline/readline.c \
	)

SRC_TEENSY = $(addprefix core/,\
	usb_desc.c \
	usb_mem.c \
	usb_serial.c \
	yield.c \
	)

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(STM_SRC_C:.c=.o) $(STM_SRC_S:.s=.o) $(SRC_TEENSY:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))
OBJ += $(BUILD)/pins_gen.o

all: hex
hex: $(BUILD)/micropython-mz.hex

ifeq ($(ARDUINO),)
post_compile: $(BUILD)/micropython-mz.hex
	$(ECHO) "Please define ARDUINO (where TeensyDuino is installed)"
	exit 1

reboot:
	$(ECHO) "Please define ARDUINO (where TeensyDuino is installed)"
	exit 1

else
TOOLS_PATH = $(ARDUINO)/hardware/tools

post_compile: $(BUILD)/micropython-mz.hex
	$(ECHO) "Preparing $@ for upload"
	$(Q)$(TOOLS_PATH)/teensy_post_compile -file="$(basename $(<F))" -path="$(abspath $(<D))" -tools="$(TOOLS_PATH)"

reboot:
	$(ECHO) "REBOOT"
	-$(Q)$(TOOLS_PATH)/teensy_reboot
endif

.PHONY: deploy
deploy: post_compile reboot

#was 	$(Q)$(CC) $(LDFLAGS) -o "$@" -Wl, $(OBJ) $(LIBS)
$(BUILD)/micropython.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) $@

ifeq ($(MEMZIP_DIR),)
MEMZIP_DIR = memzip_files
endif

$(BUILD)/micropython-mz.hex: $(BUILD)/micropython.hex $(shell find ${MEMZIP_DIR} -type f)
	@$(ECHO) "Creating $@"
	$(Q)./add-memzip.sh $< $@ ${MEMZIP_DIR}

$(BUILD)/%.hex: $(BUILD)/%.elf
	$(ECHO) "HEX $<"
	$(Q)$(OBJCOPY) -O ihex -R .eeprom "$<" "$@"


MAKE_PINS = make-pins.py
BOARD_PINS = boards/$(BOARD)/$(BOARD)_pins.csv
PREFIX_FILE = boards/mk20dx256_prefix.c
GEN_PINS_SRC = $(BUILD)/pins_gen.c
GEN_PINS_HDR = $(HEADER_BUILD)/pins.h
GEN_PINS_QSTR = $(BUILD)/pins_qstr.h
GEN_PINS_AF_CONST = $(HEADER_BUILD)/pins_af_const.h
GEN_PINS_AF_PY = $(BUILD)/pins_af.py

# Making OBJ use an order-only depenedency on the generated pins.h file
# has the side effect of making the pins.h file before we actually compile
# any of the objects. The normal dependency generation will deal with the
# case when pins.h is modified. But when it doesn't exist, we don't know
# which source files might need it.
$(OBJ): | $(HEADER_BUILD)/pins.h

# Use a pattern rule here so that make will only call make-pins.py once to make
# both pins_$(BOARD).c and pins.h
$(BUILD)/%_gen.c $(HEADER_BUILD)/%.h $(HEADER_BUILD)/%_af_const.h $(BUILD)/%_qstr.h: boards/$(BOARD)/teensy_%.csv $(MAKE_PINS) $(AF_FILE) $(PREFIX_FILE) | $(HEADER_BUILD)
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(MAKE_PINS) --board $(BOARD_PINS) --af $(AF_FILE) --prefix $(PREFIX_FILE) --hdr $(GEN_PINS_HDR) --qstr $(GEN_PINS_QSTR) --af-const $(GEN_PINS_AF_CONST) --af-py $(GEN_PINS_AF_PY) > $(GEN_PINS_SRC)

$(BUILD)/pins_gen.o: $(BUILD)/pins_gen.c
	$(call compile_c)

$(BUILD)/%.pp: $(BUILD)/%.c
	$(ECHO) "PreProcess $<"
	$(Q)$(CC) $(CFLAGS) -E -Wp,-C,-dD,-dI -o $@ $<

include ../../py/mkrules.mk
