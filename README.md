Build a multi-tasking OS kernel for ARM from scratch

Prerequisites
-------------
- [QEMU with an STM32 microcontroller implementation](http://beckus.github.io/qemu_stm32/)
  - Build instructions
```
./configure --disable-werror --enable-debug \
    --target-list="arm-softmmu" \
    --extra-cflags=-DSTM32_UART_NO_BAUD_DELAY \
    --extra-cflags=-DSTM32_UART_ENABLE_OVERRUN \
    --disable-gtk
make
```
- [GNU Toolchain for ARM](https://launchpad.net/gcc-arm-embedded)
- Set `$PATH` accordingly

Steps
-----
* `00-Semihosting`
  - Minimal semihosted ARM Cortex-M "Hello World"
* `01a-HelloWorld`
  - Enable STM32 USART to print trivial greetings
* `01b-HelloWorld`
  - Almost identical to the previous one but improved startup routines
* `02-ContextSwitch-1`
  - Basic switch between user and kernel mode
* `03-ContextSwitch-2`
  - system call is introduced as an effective method to return to kernel
* `04-Multitasking`
  - Two user tasks are interatively switching
* `05-TimerInterrupt`
  - Enable SysTick for future scheduler implementation
* `06-Preemptive`
  - Basic preemptive scheduling
* `07-Threads`
  - Implement user-level threads
* `08-CMSIS`
  - Illustrate hardware abstraction layer (HAL) by introducing CMSIS
  - Both emulator (based on stm32-p103) and real device (STM32F429i discovery)
    are supported.
* `09a-dueOS`
  - Arduino Due
* `09b-PSoC`
  - PSoC
* `10-mOS`
  - mbed
  - K64F
  - Bootloader
* `11a-RPi3`
  - Raspberry Pi 3 with arm64
* `11b-iPod`
  - iPod
* `12-m1n1`
  - MacBook Air M1
  
Building and Verification
-------------------------
* Changes the current working directory to the specified one and then
```
make
make qemu
```

Guide to Hacking 08-CMSIS
=========================================
08-CMSIS implements preemptive round-robin scheduling with user-level threads
for STM32F429i-Discovery (real device) and STM32-P103 (qemu).

## Get the dependencies:
```
git submodule init
git submodule update
```

Install additional utilities:
- If you would like to verify the kernel on STM32F29i-Discovery, you should
  install:
  * [st-link](https://github.com/texane/stlink) or [OpenOCD](http://openocd.org/)
  - [NeoCon](http://wiki.openmoko.org/wiki/NeoCon) or [screen](http://www.commandlinefu.com/commands/view/6130/use-screen-as-a-terminal-emulator-to-connect-to-serial-consoles)


## Support Devices:
- [STM32F429i-Discovery](http://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-eval-tools/stm32-mcu-eval-tools/stm32-mcu-discovery-kits/32f429idiscovery.html)
  - [Details in Chinese by NCKU](http://wiki.csie.ncku.edu.tw/embedded/STM32F429)
  - STM32F429i-Discovery uses `USART1(Tx=PA9,Rx=PA10,baud rate=115200)` as default serial port here.
    - You would need a terminal emulator, such as `screen`
      - Installation on Ubuntu / Debian based systems:
      `sudo apt-get install screen`
      - Then, attach the device file where a serial to USB converter is attached:
      `screen /dev/ttyUSB0 115200 8n1`
      - Once you want to quit screen, press: `Ctrl-a` then `k`
- [K64F](https://www.nxp.com/design/development-boards/freedom-development-boards/mcu-boards/freedom-development-platform-for-kinetis-k64-k63-and-k24-mcus:FRDM-K64F)
- [Raspberry Pi](https://www.raspberrypi.com/products/raspberry-pi-3-model-b-plus/)
- [Arduino Due](https://store.arduino.cc/products/arduino-due)
- [MacBook Air M1](https://en.wikipedia.org/wiki/Apple_M1)

## Available commands:
**Overall**
  - `make all`
    - Build all target's bin,elf,objdump files in the "release" directory.
    - NOTE: `make` doe NOT equal to `make all` here because Makefile uses
      `eval` for `targets`.
  - `make clean`
    - Remove the entire "release" directory.

**STM32-P103(QEMU)**
  - `make p103` or `make target PLAT=p103`
    - Build "p103.bin"
  - `make qemu`
    - Build "p103.bin" and run QEMU automatically.
  - `make qemu_GDBstub`
    - Build "p103.bin" and run QEMU with GDB stub and wait for remote
      GDB automatically.
  - `make qemu_GDBconnect`
    - Open remote GDB to connect to the QEMU GDB stub with port:1234 (the
      default port).

**STM32F429i-Discovery(physical device)**
  - `make f429disco` or `make target PLAT=f429disco`
    - Build "f429disco.bin"
  - `make st-flash`
    - Build "f429disco.bin" and flash the binary into STM32F429 with st-link.
  - `make st-erase`
    - Sometimes, STM32F429i-Discovery will not be able to flash new binary
      file, then you will need to erase flash memory with st-link.
    - Erase the entire flash on STM32F429.
  - `make gdb_ST-UTIL`
    - Using GDB with ST-LINK on STM32F429.
    - Remember to open another terminal,and type "st-util" to open
      "STLINK GDB Server"

## Directory Tree:
- core
  - Hardware independent source and header files.
- platform
  - Hardware dependent source and header files.
- cmsis
  - With cmsis,porting would be much easier!
- release
  - This directory will be created after "Target" in Makefile is called.
  - Containing the elf,bin,objdump files in corresponding directory.
  - `make clean` will remove the entire directory,do not put personal files
    inside it!


## Porting Guide:
You should know what [CMSIS](http://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php) is and why it saves us a lot of efforts.

`cmsis` is a submodule from [TibaChang/cmsis](https://github.com/TibaChang/cmsis), maintained by Jia-Rung Chang.

The full project can be divided into two layer:
- hardware-dependent part (HAL)
  - "platform" directory
- hardware-indepentent part
  - "core" directory
    

Licensing
---------
`armOS` is freely redistributable under the two-clause BSD License.
Use of this source code is governed by a BSD-style license that can be found
in the `LICENSE` file.

Reference
---------
* [Baking Pi – Operating Systems Development](http://www.cl.cam.ac.uk/projects/raspberrypi/tutorials/os/)
* [OSDev - Raspberry Pi Bare Bones](https://wiki.osdev.org/Raspberry_Pi_Bare_Bones)
