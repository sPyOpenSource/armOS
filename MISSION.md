# Mission: Port armOS to new ARM Cortex-M chips

## Why
I maintain armOS, a MicroPython fork running on ARM Cortex-M microcontrollers (currently sam3x, stm32f1). I want to be able to port it to new MCUs — understand memory maps, vector tables, boot sequences, linker scripts, and vendor abstraction layers so each new port doesn't feel like guesswork.

## What success looks like
- I can read a new chip's datasheet + reference manual and know exactly which files need changing in the port tree.
- I understand what the linker script is doing and can write one from scratch for a new MCU.
- I can trace execution from power-on through reset handler, C runtime init, to `main()`.
- I can decide whether to use CMSIS, a vendor HAL, or raw register access — and implement any of them.

## Constraints
- Using `arm-none-eabi-gcc 4.8.3-2014q1` (Arduino toolchain)
- Target is Cortex-M3/M4 primarily
- No debugger/probe available — learning through reading and static analysis
