/* Freescale linker file from KDS300 specified project using FRDM-K64
 * SDK\platform\devices\MK64F12\linker\gcc\ MK64FN1M0xxx12_flash.ld MK64FN1M0xxx12_ram.ld
 * Device MKF64FN1M0VLL12
 */
 /*
** ###################################################################
**     Processors:          MK64FN1M0VDC12
**                          MK64FN1M0VLL12
**                          MK64FN1M0VLQ12
**                          MK64FN1M0VMD12
**
**     Compiler:            GNU C Compiler
**     Reference manual:    K64P144M120SF5RM, Rev.2, January 2014
**     Version:             rev. 2.7, 2014-10-14
**     Build:               b141113
**
**     Abstract:
**         Linker file for the GNU C Compiler
**
**     Copyright (c) 2014 Freescale Semiconductor, Inc.
**     All rights reserved.
**
**     Redistribution and use in source and binary forms, with or without modification,
**     are permitted provided that the following conditions are met:
**
**     o Redistributions of source code must retain the above copyright notice, this list
**       of conditions and the following disclaimer.
**
**     o Redistributions in binary form must reproduce the above copyright notice, this
**       list of conditions and the following disclaimer in the documentation and/or
**       other materials provided with the distribution.
**
**     o Neither the name of Freescale Semiconductor, Inc. nor the names of its
**       contributors may be used to endorse or promote products derived from this
**       software without specific prior written permission.
**
**     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
**     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
**     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
**     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
**     ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
**     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
**     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
**     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**     http:                 www.freescale.com
**     mail:                 support@freescale.com
**
** ###################################################################
*/

/* Sect 3.5.1.2 Flash Block0 and Block1
* Flash Block0 0x0000_0000 0x0007_FFFF P-Flash, exceptions in first 1024byte
*   note Flash_Config 0x0000400 -->0410
*Flash Block1 0x0008_0000 0x000F_FFFF FlexNVM/P-Flash
* RAM Lower  0x1FFF_0000 0x1FFF_FFFF   65,536bytes
* Ram Higher 0x2000_0000 0x2002_FFFF  196,608bytes
*        RAM (256*1024=262,144bytes)  262,144bytes
*/
MEMORY
{
        FLASH_B0 (rx) : ORIGIN = 0x00000000, LENGTH = 0x00100000
        RAML  (rwx) : ORIGIN = 0x1FFF0000, LENGTH = 256K
}

/* produce a link error if there is not this amount of RAM for these sections */
_minimum_stack_size = 16K;
_minimum_heap_size = 16K;

/* INCLUDE MK64FN1M0xxx12_flash.ld  */


/* Specify the memory areas -definitions information  * /
MEMORY
{
  m_interrupts          (RX)  : ORIGIN = 0x00000000, LENGTH = 0x00000400
  m_flash_config        (RX)  : ORIGIN = 0x00000400, LENGTH = 0x00000010
  m_text                (RX)  : ORIGIN = 0x00000410, LENGTH = 0x000FFBF0
  m_data                (RW)  : ORIGIN = 0x1FFF0000, LENGTH = 0x00010000
  m_data_2              (RW)  : ORIGIN = 0x20000000, LENGTH = 0x00030000
} */

/* Define output sections */
SECTIONS
{
    .text : {
        . = 0;
        KEEP(*(.vectors))
        *(.startup*)
        /* TODO: does linker detect startup overflow onto flashconfig? */
        . = 0x400;
        KEEP(*(.flashconfig*)) /* defined in code mk20dx128.c */
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        KEEP(*(.init))
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    } > FLASH_B0 = 0xFF

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH_B0
    _etext = .;

    .usbdescriptortable (NOLOAD) : {
        /* . = ORIGIN(RAML); */
        . = ALIGN(512);
        *(.usbdescriptortable*)
    } > RAML

    .dmabuffers (NOLOAD) : {
        . = ALIGN(4);
        *(.dmabuffers*)
    } > RAML

    .usbbuffers (NOLOAD) : {
        . = ALIGN(4);
        *(.usbbuffers*)
    } > RAML

    /* used by the startup to initialize data */
    _sidata = LOADADDR(.data);

    .data : AT (_etext) {
        . = ALIGN(4);
        _sdata = .;
        _ram_start = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAML

        /*
     * _staticfs is the place in flash where the static filesystem which
     * is concatenated to the .hex file will wind up.
         */
    _staticfs = LOADADDR(.data) + SIZEOF(.data);

    .noinit (NOLOAD) : {
        *(.noinit*)
    } > RAML

    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end = .;
    } > RAML

    /* this is to define the start of the heap, and make sure we have a minimum size */
    .heap :
        {
        . = ALIGN(4);
        _heap_start = .;    /* define a global symbol at heap start */
        . = . + _minimum_heap_size;
    } >RAML

    /* this just checks there is enough RAM for the stack */
    .stack :
        {
        . = ALIGN(4);
        . = . + _minimum_stack_size;
        . = ALIGN(4);
    } >RAML

    _estack = ORIGIN(RAML) + LENGTH(RAML);
    _ram_end = ORIGIN(RAML) + LENGTH(RAML);
    _heap_end = ORIGIN(RAML) + 0x10000; /* was 0x0e000 */
}
