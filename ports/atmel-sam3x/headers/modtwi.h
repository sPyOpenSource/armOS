
#ifndef MODTWI_H
#define MODTWI_H

#include "asf.h"
#include "modpinmap.h"


#define TIMEOUT  100000	// 100 KHz default speed
#define BUFFER_LEN 64	// Internal TX buffer
#define MASTER 	1
#define SLAVE 	2 // not implemented yet


typedef struct twi_interface {
	uint8_t tx_buffer[BUFFER_LEN];
	uint8_t tx_index;
	uint8_t tx_buff_len;
}twi_interface;


extern const mp_obj_type_t i2c_type;


// function prototypes

uint32_t twi_setup_master(Twi *ptwi, uint32_t speed, uint8_t address);
uint8_t twi_readfrom(Twi *ptwi, uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize);
bool twi_wait_received(Twi *ptwi, uint32_t timeout);
void send_stop_bit(Twi *ptwi);
bool twi_wait_transfer(Twi *ptwi, uint32_t timeout);
void twi_start_read(Twi *ptwi, uint8_t address,uint32_t iaddress, uint32_t isize);
uint32_t get_status(Twi *ptwi);
void twi_stop(Twi *ptwi);
uint8_t returns(Twi *ptwi, uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize);
uint8_t twi_writeto(Twi *ptwi, uint8_t txaddress, uint32_t iaddress, uint8_t isize, const uint8_t *data, uint8_t quantity);
void twi_start_write(Twi *ptwi, uint8_t address, uint32_t iaddress, uint8_t isize, uint8_t byte);
bool twi_wait_byte_sent( Twi *ptwi, uint32_t timeout);
void twi_write(Twi *ptwi, uint8_t byte);
uint8_t twi_readto(Twi *ptwi, uint8_t address, uint8_t quantity, uint8_t *data, bool stop);
void twi_ini_bus(Twi *ptwi);
void i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
#endif