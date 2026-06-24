# Resources

## ARM Architecture Reference

### ARMv7-M Architecture Reference Manual (DDI 0403E)
- **Why**: The authoritative spec for Cortex-M3/M4. Describes programmer's model, memory map, exception model, instruction set.
- **Trust**: high

### Cortex-M3 Technical Reference Manual (DDI 0337)
- **Why**: Cortex-M3 specific: NVIC, MPU, debug, bus matrix. Needed to understand boot and interrupt behavior.
- **Trust**: high

### Cortex-M4 Technical Reference Manual (DDI 0439)
- **Why**: Same as above for M4, plus FPU details.
- **Trust**: high

## Toolchain & Linker

### GNU LD Manual
- **URL**: https://sourceware.org/binutils/docs/ld/
- **Why**: Understanding linker scripts is essential for porting. MEMORY, SECTIONS, location counter, PROVIDE.
- **Trust**: high

### ARM Embedded GCC Linker Script Guide (from ARM-software/abi-aa)
- **Trust**: high

## Porting Examples

### MicroPython porting docs
- **URL**: https://docs.micropython.org/en/latest/develop/ports.html
- **Why**: Shows the structure of a MicroPython port — what belongs in `mphalport.h`, `mpconfigport.h`, `Makefile`.
- **Trust**: high

### Existing armOS ports (sam3x, bluepill, stm32)
- **Why**: Concrete examples of working ports in this exact codebase. Good for comparison when creating new ports.
- **Trust**: high
