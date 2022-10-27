
#ifndef DUE_MPHAL_H
#define DUE_MPHAL_H


#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "system_sam3x.h"
#include "pmc.h"
#include "asf.h"
#include "ASF/sam/boards/arduino_due_x/arduino_due_x.h"

#define CONF_BOARD_USB_PORT

#define BUFFER_SIZE 128

#define USB_REPL (1)

#define ENABLE_TRNG (1)


extern int _end;

// _sbrk is used in main() to return heap size for Micropython GC.
extern caddr_t _sbrk( int incr ) ;


// These functions are required by libc and never used in micropython (yet).
extern int link( char *cOld, char *cNew ) ;

extern int _close( int file ) ;

extern int _fstat( int file, struct stat *st ) ;

extern int _isatty( int file ) ;

extern int _lseek( int file, int ptr, int dir ) ;

extern int _read(int file, char *ptr, int len) ;

extern int _write( int file, char *ptr, int len ) ;


void init_system(void);
void init_uart(void);
uint8_t receive_usb(void);
uint8_t receive_uart(void);
void put_char (const uint8_t c);
void mp_hal_set_interrupt_char(int c);
void ADC_init();
#endif