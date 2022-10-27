include ../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include ../../py/py.mk

CROSS_COMPILE = /Users/xuyi/Library/Arduino15/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-
ROOT := $(realpath $(shell dirname '$(dir $(lastword $(MAKEFILE_LIST)))'))
INC += -I.
INC += -I../..
INC += -I../../lib/libm
INC += -I./ASF/
INC += -I./ASF/common/
INC += -I./ASF/common/boards/
INC += -I./ASF/services
INC += -I./IASF/common/services/sleepmgr/
INC += -I./IASF/common/services/sleepmgr/sam/
INC += -I./IASF/common/services/storage/ctrl_access/
INC += -I./ASF/common/services/usb/
INC += -I./ASF/common/services/usb/class/
INC += -I./ASF/common/services/usb/class/cdc/
INC += -I./ASF/common/services/usb/class/cdc/device/
INC += -I./ASF/common/services/usb/udc/
INC += -I./ASF/common/services/usb/class/msc/
INC += -I./ASF/common/services/usb/class/msc/device/
INC += -I./ASF/common/utils
INC += -I./ASF/common/sam/drivers/uotghs/
INC += -I./ASF/common/sam/utils
INC += -I./ASF/common/sam/utils/preprocessor
INC += -I./ASF/common/sam/utils/header_files
INC += -I./ASF/Config
INC += -I./ASF/sam/boards/
INC += -I./ASF/sam/
INC += -I./ASF/common/services/gpio/sam_gpio/
INC += -I./ASF/sam/utils/cmsis/sam3x/source/templates
INC += -I./ASF/common/services/clock/
INC += -I./ASF/common/services/clock/sam3x/
INC += -I./ASF/sam/services/gpio/
INC += -I./ASF/common/services/serial/
INC += -I./ASF/common/services/serial/sam_uart/
INC += -I./ASF/sam/utils/cmsis/sam3x/include
INC += -I./ASF/thirdparty/CMSIS/Include/
INC += -I./ASF/common/utils/interrupt/
INC += -I./ASF/common/utils/stdio/
INC += -I./ASF/common/utils/stdio/stdio_serial/
INC += -I./ASF/boards/arduino_due_x/
INC += -I./ASF/sam/drivers/
INC += -I./ASF/sam/drivers/pio/
INC += -I./ASF/sam/drivers/rtc/
INC += -I./ASF/sam/drivers/pmc/
INC += -I./ASF/sam/drivers/uart/
INC += -I./ASF/sam/drivers/usart/
INC += -I./ASF/sam/utils/
INC += -I./ASF/sam/utils/cmsis/
INC += -I./ASF/sam/utils/cmsis/sam3x/
INC += -I./ASF/sam/utils/cmsis/sam3x/include/
INC += -I./ASF/sam/utils/cmsis/sam3x/include/component/
INC += -I./ASF/sam/utils/cmsis/sam3x/include/instance/
INC += -I./ASF/sam/utils/cmsis/sam3x/include/pio/
INC += -I./ASF/sam/utils/cmsis/sam3x/source/
INC += -I./ASF/sam/utils/cmsis/sam3x/source/templates/
INC += -I./ASF/sam/utils/cmsis/sam3x/include/templates/gcc/
INC += -I./ASF/thirdparty/CMSIS/Include/
INC += -I./ASF/common/services/delay/
INC += -I./ASF/common/services/delay/sam/
INC += -I./ASF/common/services/fifo/
INC += -I./ASF/sam/drivers/adc/
INC += -I./ASF/sam/drivers/tc/
INC += -I./ASF/sam/drivers/dacc/
INC += -I./ASF/arduino/
INC += -I./ASF/sam/drivers/pwm/
INC += -I./ASF/sam/drivers/twi/
INC += -I./ASF/common/services/twi/sam_twi/
INC += -I./ASF/sam/drivers/rstc/
INC += -I./ASF/sam/drivers/trng/

INC += $(addprefix -IASF/sam/, \
		drivers/uotghs/ \
		utils/ \
		drivers/adc/ \
		drivers/tc/ \
		drivers/pwm/ \
		drivers/dacc/ \
		utils/preprocessor/ \
		utils/header_files/ \
		drivers/pmc \
		drivers/pio \
		drivers/rtc/ \
		drivers/uart/ \
		drivers/uotghs/ \
		drivers/usart/ \
		drivers/twi/ \
		drivers/rstc/ \
		drivers/trng/ \
		utils/cmsis/sam3x/include/ )


INC += $(addprefix -IASF/common/, \
		services/sleepmgr/ \
		services/utils \
		services/utils/interrupt \
		services/sleepmgr/sam \
		services/clock/ \
		services/clock/sam3x \
		services/gpio \
		services/delay/ \
		services/delay/sam/ \
		utils/stdio/ \
		utils/stdio/stdio_serial/ \
		services/serial \
		serial/sam_uart/ \
		utils/ \
		utils/interrupt/ \
		boards/ \
		services/sam_twi/ \
		services/fifo/ \
		services/usb/ \
		services/usb/class/ \
		services/usb/class/cdc/ \
		services/usb/class/cdc/device/ \
		services/usb/udc/ \
		services/usb/class/msc/ \
		services/usb/class/msc/device/ 	\ )


INC += $(addprefix -IASF,Config/ )
INC += $(addprefix -IASF,arduino/ )

INC += -I$(BUILD)


#GEN_PINS_QSTR = $(BUILD)/pins_qstr.h

CFLAGS_CORTEX_M4 = -O2 -mthumb -mtune=cortex-m3 -mcpu=cortex-m3 -msoft-float -mfloat-abi=soft -fsingle-precision-constant -Wdouble-promotion --param max-inline-insns-single=500 -mlong-calls -ffunction-sections -fdata-sections
CFLAGS = $(INC) -Wall -std=c99 -nostdlib -D__SAM3X8E__  -lm -lc  $(CFLAGS_CORTEX_M4)

#Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
endif

LDFLAGS =  -T flash.ld -Wl,-Map=$@.map -Wl,--cref -msoft-float -mfloat-abi=soft
LIBS    =  -lm libm.a $(BUILD)/firm.a $(BUILD)/py.a

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


SRC_ASF = \
	ASF/common/services/sleepmgr/sam/sleepmgr.c \
	ASF/common/services/delay/sam/cycle_counter.c \
	ASF/sam/utils/cmsis/sam3x/source/templates/exceptions.c \
	ASF/common/services/clock/sam3x/sysclk.c \
	ASF/sam/drivers/pmc/sleep.c \
	ASF/sam/utils/cmsis/sam3x/source/templates/system_sam3x.c \
	ASF/sam/drivers/pmc/pmc.c \
	ASF/sam/drivers/rtc/rtc.c \
	ASF/sam/drivers/uart/uart.c \
	ASF/sam/drivers/usart/usart.c \
	ASF/sam/utils/cmsis/sam3x/source/templates/system_sam3x.c \
	ASF/sam/utils/cmsis/sam3x/source/templates/gcc/startup_sam3x.c \
	ASF/common/utils/interrupt/interrupt_sam_nvic.c \
	ASF/sam/drivers/pio/pio_handler.c \
	ASF/sam/drivers/pio/pio.c \
	ASF/sam/drivers/adc/adc.c \
	ASF/sam/drivers/tc/tc.c \
	ASF/sam/drivers/dacc/dacc.c \
	ASF/arduino/pwmc.c \
	ASF/sam/drivers/pwm/pwm.c \
	ASF/sam/drivers/twi/twi.c \
	ASF/sam/drivers/rstc/rstc.c \
	ASF/sam/boards/arduino_due_x/init.c \
	ASF/common/services/usb/class/cdc/device/udi_cdc.c \
	ASF/common/services/usb/class/cdc/device/udi_cdc_desc.c \
	ASF/common/services/usb/udc/udc.c \
	ASF/sam/drivers/uotghs/uotghs_device.c \
	ASF/sam/drivers/trng/trng.c \

#	ASF/common/services/usb/class/msc/device/udi_msc.c \
#	ASF/common/services/usb/class/msc/device/udi_msc_desc.c \


OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o ) )
OBJ += $(addprefix $(BUILD)/, $(SRC_ASF:.c=.o))
#OBJ += $(PY_O) $(addprefix $(BUILD)/, $(SRC_OBJ:.c=.o))
LD = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar


SRC_QSTR += $(SRC_C)


all: $(BUILD)/firmware.bin

$(BUILD)/firmware.elf:$(OBJ)
	$(ECHO) "Compiling $@"
	$(AR) rcs $(BUILD)/firm.a $(BUILD)/main.o
	$(AR) rcs $(BUILD)/py.a $(filter-out build/main.o,$(OBJ))

	$(ECHO) "Link $@"
	$(LD)   -Os -Wl,--gc-sections -mcpu=cortex-m3 $(LDFLAGS) -o $@ -lgcc -lm -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--entry=Reset_Handler -Wl,--start-group -u _sbrk -u link -u _close -u _fstat -u _isatty -u _lseek -u _read -u _write -u _exit -u kill -u _getpid $(LIBS) -Wl,--end-group
	$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
		$(ECHO) "Creating $@"
		$(CROSS_COMPILE)objcopy -O binary $(BUILD)/firmware.elf $(BUILD)/firmware.bin


upload: %(BUILD)/firmware.bin
%(BUILD)/firmware.bin:
	stty -F /dev/$(port) cs8 1200 hupcl
	./tools/bossac --port=${port} -U true -e -w -b $(BUILD)/firmware.bin -R

include ../../py/mkrules.mk
