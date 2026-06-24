# 2026-06-24: ARM Cortex-M Memory Map and Vector Table

## Context
First lesson on ARM architecture. User wants to port armOS to new Cortex-M chips. Needed to understand the fundamental address space layout before anything else.

## Key Insight
The Cortex-M memory map is vendor-independent in its regions (Code 0x0, SRAM 0x20000000, Peripheral 0x40000000, System 0xE0000000). Each chip vendor picks a flash base within the Code region, and the boot alias mechanism makes it appear at 0x0 for vector table fetch. This explains why the linker script's MEMORY block always has SRAM at 0x20000000 but flash at a vendor-specific address (0x08000000 for STM32, 0x00080000 for SAM3X).

## Links
- Lesson: [0001-cortex-m-memory-map](./lessons/0001-cortex-m-memory-map.html)
- Reference: [cortex-m-memory-map](./reference/cortex-m-memory-map.html)
