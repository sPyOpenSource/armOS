#!/usr/bin/make
# makefile for the arduino due (works with arduino IDE 1.6.11)
#
# USAGE: put this file in the same dir as your .ino file is.
# configure the PORT variable and ADIR at the top of the file
# to match your local configuration.
# Type make upload to compile and upload.
# Type make monitor to watch the serial port with gnu screen.
#
# TODO:
#  * support libraries
#  * handle possibly missing files in the currently hard coded ar step
#    when assembling core.a together.
#  * see what to do about the $(SAM)/cores/arduino/wiring_pulse_asm.S" add -x assembler-with-cpp
#    and this one: $(SAM)/cores/arduino/avr/dtostrf.c
#
# LICENSE: GPLv2 or later (at your option)
#
# This file can be found at https://github.com/pauldreik/arduino-due-makefile
#
# By Paul Dreik http://www.pauldreik.se/
# 20130503 initial version
# 20160924 updated to work with arduino 1.6.11

#user specific settings:
#where to find the IDE
ADIR:=./
#which serial port to use (add a file with SUBSYSTEMS=="usb",
#ATTRS{product}=="Arduino Due Prog. Port", ATTRS{idProduct}=="003d",
#ATTRS{idVendor}=="2341", SYMLINK+="arduino_due" in /etc/udev/rules.d/
#to get this working). Do not prefix the port with /dev/, just take
#the basename.
PORT:=ttyACM0
#if you want to verify the bossac upload, define this to -v
VERIFY:=

#end of user configuration.

#then some general settings. They should not be necessary to modify.
BASE:=./gcc-arm-none-eabi-6-2017-q2-update/bin/
CXX:=$(BASE)arm-none-eabi-g++
CC:=$(BASE)arm-none-eabi-gcc
OBJCOPY:=$(BASE)arm-none-eabi-objcopy
AR:=$(BASE)arm-none-eabi-ar

C:=$(CC)
SAM:=$(ADIR)
TMPDIR:=$(PWD)/build

#all these values are hard coded and should maybe be configured somehow else,
#like olikraus does in his makefile.
DEFINES:=-Dprintf=iprintf -DF_CPU=84000000  -DARDUINO=10611 -D__SAM3X8E__ -DUSB_PID=0x003e -DUSB_VID=0x2341 -DUSBCON \
         -DARDUINO_SAM_DUE -DARDUINO_ARCH_SAM '-DUSB_MANUFACTURER="Arduino LLC"' '-DUSB_PRODUCT="Arduino Due"'

INCLUDES:=-I$(ADIR)/libraries/arduino_due -I$(ADIR)/Includes/sam \
          -I$(ADIR)/libraries/Receiver -I$(ADIR)/libraries/Sensors \
          -I$(ADIR)/libraries/Adafruit_master -I$(ADIR)/libraries/Arduino-PID-Library \
          -I$(ADIR)/libraries/KalmanFilter-master -I$(ADIR)/libraries/LSM303 \
          -I$(ADIR)/Includes -I$(ADIR)/libraries/AIDrone -I$(ADIR)/libraries/UnitTest \
          -I$(ADIR)/libraries/Adafruit_GFX_Library -I$(ADIR)/libraries/Adafruit_ST7735_and_ST7789_Library \
	  			-I$(ADIR)/libraries/SD/src -I$(ADIR)/libraries/HID/src -I$(ADIR)/libraries/SPI/src \
					-I$(ADIR)/libraries/USB

#also include the current dir for convenience
INCLUDES += -I.

#compilation flags common to both c and c++
COMMON_FLAGS:=-g -Os -w -ffunction-sections -fdata-sections -nostdlib \
              --param max-inline-insns-single=500 -mcpu=cortex-m3 -mthumb \
              -fno-threadsafe-statics

#for compiling c (do not warn, this is not our code)
CFLAGS:=$(COMMON_FLAGS) -std=gnu11

#for compiling c++
CXXFLAGS:=$(COMMON_FLAGS) -fno-rtti -fno-exceptions -std=gnu++11 -Wall -Wextra

#let the results be named after the project
PROJNAME:=drone

#These source files are the ones forming core.a
CORESRCXX:=$(shell ls ${SAM}/libraries/USB/*.cpp ${SAM}/libraries/HID/src/*.cpp ${SAM}/libraries/SD/src/*.cpp ${SAM}/libraries/SD/src/utility/*.cpp ${SAM}/libraries/SPI/src/*.cpp ${SAM}/libraries/Adafruit_GFX_Library/*.cpp ${SAM}/libraries/Adafruit_ST7735_and_ST7789_Library/*.cpp ${SAM}/src/*.cpp ${SAM}/libraries/USB/*.cpp ${SAM}/libraries/arduino_due/variant.cpp ${SAM}/libraries/Sensors/*.cpp ${SAM}/libraries/Receiver/*.cpp ${SAM}/libraries/LSM303/*.cpp ${SAM}/libraries/KalmanFilter-master/*.cpp ${SAM}/libraries/Arduino-PID-Library/*.cpp ${SAM}/libraries/AIDrone/*.cpp ${SAM}/libraries/Adafruit_master/*.cpp ${SAM}/libraries/UnitTest/*.cpp)
CORESRC:=$(shell ls ${SAM}/src/*.c)

#hey this one is needed too: $(SAM)/cores/arduino/wiring_pulse_asm.S" add -x assembler-with-cpp
#and this one: /1.6.9/cores/arduino/avr/dtostrf.c but it seems to work
#anyway, probably because I do not use that functionality.

#convert the core source files to object files. assume no clashes.
COREOBJSXX:=$(addprefix $(TMPDIR)/core/,$(notdir $(CORESRCXX)) )
COREOBJSXX:=$(addsuffix .o,$(COREOBJSXX))
COREOBJS:=$(addprefix $(TMPDIR)/core/,$(notdir $(CORESRC)) )
COREOBJS:=$(addsuffix .o,$(COREOBJS))

default: $(TMPDIR)/$(PROJNAME).elf $(TMPDIR)/$(PROJNAME).bin

#This is a make rule template to create object files from the source files.
# arg 1=src file
# arg 2=object file
# arg 3= XX if c++, empty if c
define OBJ_template
$(2): $(1)
	$(C$(3)) -MD -c $(C$(3)FLAGS) $(DEFINES) $(INCLUDES) $(1) -o $(2)
endef

#now invoke the template both for c++ sources
$(foreach src,$(CORESRCXX), $(eval $(call OBJ_template,$(src),$(addsuffix .o,$(addprefix $(TMPDIR)/core/,$(notdir $(src)))),XX) ) )

#...and for c sources:
$(foreach src,$(CORESRC), $(eval $(call OBJ_template,$(src),$(addsuffix .o,$(addprefix $(TMPDIR)/core/,$(notdir $(src)))),) ) )

#and our own c++ sources
$(foreach src,$(MYSRCFILES), $(eval $(call OBJ_template,$(src),$(addsuffix .o,$(addprefix $(TMPDIR)/,$(notdir $(src)))),XX) ) )

clean:
	test ! -d $(TMPDIR) || rm -rf $(TMPDIR)

.PHONY: upload default

$(TMPDIR):
	mkdir -p $(TMPDIR)

$(TMPDIR)/core:
	mkdir -p $(TMPDIR)/core

#create the core library from the core objects. Do this EXACTLY as the
#arduino IDE does it, seems *really* picky about this.
#Sorry for the hard coding.
$(TMPDIR)/core.a: $(TMPDIR)/core $(COREOBJS) $(COREOBJSXX)
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/wiring_shift.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/wiring_analog.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/itoa.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/cortex_handlers.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/hooks.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/wiring.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/WInterrupts.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/syscalls_sam3.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/iar_calls_sam3.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/wiring_digital.c.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Print.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/USARTClass.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/WString.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/PluggableUSB.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/USBCore.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/CDC.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/wiring_pulse.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/UARTClass.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/main.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/new.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/watchdog.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Stream.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/RingBuffer.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/IPAddress.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Reset.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/WMath.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/variant.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/AIDrone.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Adafruit_L3GD20.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/PID_v1.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Kalman.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/LSM303.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Receiver.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Sensors.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/SFE_BMP180.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Servo.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Wire.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/UnitTest.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Adafruit_ST7735.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/SPI.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/SD.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Adafruit_GFX.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Adafruit_ST77xx.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Adafruit_SPITFT.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/SdVolume.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/SdFile.cpp.o
	$(AR) rcs $(TMPDIR)/core.a $(TMPDIR)/core/Sd2Card.cpp.o

#link our own object files with core to form the elf file
$(TMPDIR)/$(PROJNAME).elf: $(TMPDIR)/core.a $(TMPDIR)/core/syscalls_sam3.c.o
	$(CC) -mcpu=cortex-m3 -mthumb -Os -Wl,--gc-sections -T$(SAM)/libraries/arduino_due/linker_scripts/gcc/flash.ld -Wl,-Map,$(TMPDIR)/main.cpp.map -o $@ -L$(TMPDIR) -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--entry=Reset_Handler -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -Wl,--start-group -u _sbrk -u link -u _close -u _fstat -u _isatty -u _lseek -u _read -u _write -u _exit -u kill -u _getpid $(TMPDIR)/core/variant.cpp.o $(SAM)/libraries/arduino_due/libsam_sam3x8e_gcc_rel.a $(TMPDIR)/core.a -Wl,--end-group -lm -gcc

#copy from the hex to our bin file (why?)
$(TMPDIR)/$(PROJNAME).bin: $(TMPDIR)/$(PROJNAME).elf
	$(OBJCOPY) -O binary $< $@

#upload to the arduino by first resetting it (stty) and the running bossac
upload: $(TMPDIR)/$(PROJNAME).bin
	stty -F /dev/$(PORT) cs8 1200 hupcl
	./bossac -i -d --port=$(PORT) -U false -e -w $(VERIFY) -b $(TMPDIR)/$(PROJNAME).bin -R

#to view the serial port with screen.
monitor:
	screen /dev/$(PORT) 115200
