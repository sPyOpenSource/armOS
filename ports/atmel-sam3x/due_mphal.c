
#include <string.h>
#include "py/headers/mphal.h"
#include "due_mphal.h"
#include "py/headers/mpstate.h"
#include "usart.h"
#include "fifo.h"
#include "conf_sleepmgr.h"

// Low level functions required for initialization of the board


#ifdef UART_REPL
    static uint8_t uart_rx_buf[BUFFER_SIZE];
    static volatile uint8_t uart_rx_buf_head=0;
    static volatile uint8_t uart_rx_buf_tail=0;
    static volatile uint8_t uart_rx_count=0;
#endif


#ifdef USB_REPL 
    static uint8_t usb_rx_buf[BUFFER_SIZE];
    static uint8_t usb_rx_buf_head  = 0;
    static uint8_t usb_rx_buf_tail = 0;
    static uint8_t usb_rx_count = 0;
#endif

uint8_t staus;
uint8_t received = 0;
static volatile bool mp_cdc_enabled = false;

void mp_keyboard_interrupt(void);
int interrupt_char;


extern caddr_t _sbrk ( int incr )
{
    static unsigned char *heap = NULL ;
    unsigned char *prev_heap ;

    if ( heap == NULL )
    {
        heap = (unsigned char *)&_end ;
    }
    prev_heap = heap;

    heap += incr ;

    return (caddr_t) prev_heap ;
}

extern int link( char *cOld, char *cNew )
{
    return -1 ;
}

extern int _close( int file)
{
    return -1 ;
}

extern int _fstat( int file, struct stat *st )
{
    st->st_mode = S_IFCHR ;

    return 0 ;
}

extern int _isatty( int file )
{
    return 1 ;
}

extern int _lseek( int file, int ptr,int dir )
{
    return 0 ;
}

extern int _read(int file, char *ptr, int len )
{
    return 0 ;
}

extern int _write( int file, char *ptr, int len )
{
  	
    return 0;
}

extern void _exit( int status )
{

    for ( ; ; ) ;
}

extern void _kill( int pid, int sig )
{
    return ;
}

extern int _getpid ( void )
{
    return -1 ;
}

void mp_cdc_set_dtr(uint8_t port, bool enabled) {

        mp_cdc_enabled  = true;
}

bool mp_cdc_enable(uint8_t port)
{
    mp_cdc_enabled = true;
    return true;
}

void mp_cdc_disable(uint8_t port)
{

    mp_cdc_enabled = false;
}


// init UART on RX0 TX0

#ifdef UART_REPL
void init_uart(void) {
	
	PMC->PMC_PCER0 = 1 << ID_UART;
    PIOA->PIO_PDR |= PIO_PA8;
    PIOA->PIO_PDR |= PIO_PA9;
    PIOA->PIO_ABSR &= ~PIO_PA8;
    PIOA->PIO_ABSR &= ~PIO_PA9;
    PIOA->PIO_PUER = PIO_PA8;
    PIOA->PIO_PUER = PIO_PA9;

    UART->UART_BRGR = 84000000 / 115200 / 16;
    UART->UART_MR = UART_MR_PAR_NO;
    UART->UART_CR = UART_CR_TXEN | UART_CR_RXEN;
    UART->UART_PTCR = UART_PTCR_TXTEN;
    UART->UART_IER =  UART_IER_RXRDY;
    NVIC_EnableIRQ(UART_IRQn); 
}
#endif

// This will be called by 'mp_hal_stdin_rx_chr' whenever there's a character present in RX buffer

#ifdef UART_REPL
int recieve_uart() {

	if(uart_rx_count == 0)
		return 0;

	cpu_irq_disable();
	int data = uart_rx_buf[uart_rx_buf_head];
	uart_rx_buf_head++;
	uart_rx_count--;
	if(uart_rx_buf_head == BUFFER_SIZE)
		uart_rx_buf_head  = 0;

	cpu_irq_enable();

	if(uart_rx_count == (BUFFER_SIZE-1))
		UART_Handler();

	return data;
}
#endif


// Interrupt Handler for UART
#ifdef UART_REPL
void UART_Handler() {
    
    	irqflags_t flags;
    	while(uart_is_rx_ready(UART))	{
    		uint8_t c;

    		flags  = cpu_irq_save();
    		if(uart_rx_count >= BUFFER_SIZE){
    			cpu_irq_restore(flags);
    			break;
    		}

    		uint8_t curr_tail = uart_rx_buf_tail;
    	//	printf("second");
    		if((BUFFER_SIZE-1) == uart_rx_buf_tail) {
                uart_rx_buf_tail = 0x00;
            } 
    		else {
    			uart_rx_buf_tail++;
            }

    		uart_rx_count++;

            // get data from UART register
    		c = UART->UART_RHR;

    		if(c == interrupt_char){
    			uart_rx_count--;
    			uart_rx_buf_tail  =  curr_tail;
    			cpu_irq_restore(flags);
    			mp_keyboard_interrupt();
    			continue;
    		}

    		uart_rx_buf[curr_tail] = c;
    	}
	
		UART->UART_IDR |= UART_IDR_TXRDY;
}
#endif


#ifdef UART_REPL
void put_char(const uint8_t c)  {

   while(!(UART->UART_SR & UART_SR_TXRDY)){}
    UART->UART_THR = c;
}
#endif


// This is adapted from Adafruit's port for SAMD21 devices

#ifdef USB_REPL
void usb_rx_notify() {
    irqflags_t flags;

    if(mp_cdc_enabled) {

        while(udi_cdc_is_rx_ready()) {
            uint8_t c;

            flags = cpu_irq_save();
            if(usb_rx_count >= BUFFER_SIZE) {
                cpu_irq_restore(flags);
                break;
            }

            uint8_t curr_tail = usb_rx_buf_tail;
            if((BUFFER_SIZE -1) == usb_rx_buf_tail) {
                usb_rx_buf_tail = 0x00;
            } else {
                usb_rx_buf_tail++;
            }

            usb_rx_count++;

            c = udi_cdc_getc();

            if(c == interrupt_char) {

                usb_rx_count--;
                usb_rx_buf_tail = curr_tail;
                cpu_irq_restore(flags);
                mp_keyboard_interrupt();
                continue;
            }

            usb_rx_buf[curr_tail] = c;
            cpu_irq_restore(flags);
        }
    }
}
#endif


#ifdef USB_REPL
uint8_t recieve_usb() {

    if(usb_rx_count ==0) {
        return 0;
    }

    cpu_irq_disable();
    uint8_t data = usb_rx_buf[usb_rx_buf_head];
    usb_rx_buf_head++;
    usb_rx_count--;

    if((BUFFER_SIZE) == usb_rx_buf_head) {
        usb_rx_buf_head =0;
    }
    cpu_irq_enable();

    if(usb_rx_count == BUFFER_SIZE - 1) {
        usb_rx_notify();
    }
    return data;
}
#endif

void mp_hal_set_interrupt_char(int c) {
        if (c != -1) {
        mp_obj_exception_clear_traceback(MP_STATE_PORT(mp_kbd_exception));
    }
    extern int interrupt_char;
    interrupt_char = c;
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}


int mp_hal_stdin_rx_chr(void){

  		for(;;) {

            #ifdef UART_REPL    
  			if(uart_rx_count > 0){
  				return recieve_uart();
  			}
            #endif
            
            #ifdef USB_REPL
            if(mp_cdc_enabled && usb_rx_count > 0) {
			     return recieve_usb();
		      }
            #endif
             sleepmgr_enter_sleep();
             
        }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len){

    #ifdef UART_REPL
	while(len--){
        put_char(*str++);
    }
    #endif
    
    #ifdef USB_REPL
    
    if(mp_cdc_enabled && udi_cdc_is_tx_ready()) {
        udi_cdc_write_buf(str, len);
    }
    #endif
    
}

mp_uint_t mp_hal_ticks_ms(void) {
    return 0;
}

void mp_hal_delay_ms(mp_uint_t ms) {
}


void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    while (len--) {
        if (*str == '\n') {
            mp_hal_stdout_tx_strn("\r", 1);
        }
        mp_hal_stdout_tx_strn(str++, 1);
    }
}

// Initialize ADC 
void ADC_init() {
  pmc_enable_periph_clk(ID_ADC);
  adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
  adc_configure_timing(ADC, 0, ADC_SETTLING_TIME_3, 1);
  adc_configure_trigger(ADC, ADC_TRIG_SW, 0); // Disable hardware trigger.
  adc_disable_interrupt(ADC, 0xFFFFFFFF); // Disable all ADC interrupts.
  adc_disable_all_channel(ADC);
}
