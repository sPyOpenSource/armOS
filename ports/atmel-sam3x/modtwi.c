
#include <string.h>

#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"

#include "asf.h"
#include <twi.h>
#include <twi_master.h>

#include "modtwi.h"
#include "modpinmap.h"


/// class I2C - a two-wire serial protocol
/// The methods are based on NEW hardware API of Micropython
/// link: https://github.com/micropython/micropython/wiki/Hardware-API
///
///
///
/// 	from machine import I2C
///
///		i2c = I2C(1)				# create on bus 1 (Arduino Due's TWI0) = ( SCL1, SDL1) = (Pin 70, 71)
///		i2c = I2C(2) 				# create on bus 2 (Arduino Due's TWI1) = ( SCL0, SDL0) = (Pin 20, 21)
///		i2c = I2C(1, 100000, 0x50)	# create on bus 1 with buadrate: 100Khz and slave address = 0x50
///		i2c.deinit()				# turn of I2C bus (stop bus clock)
///		
///		For now, I2C works only in MASTER mode
///
///
///	Available methods:
///	
///		i2c.scan()									# Search for devices present on the bus 					
///
///		i2c.readfrom(addr, nbytes, stop_bit) 		# addr: slave address
///											 		# nbytes: number of bytes you want to read 
///											 		# stop_bit: send a stop bit after reading
///
///		i2c.readfrom_into(addr, buf, stop_bit)		# addr: slave address
///													# buf: bytearray object
///													# stop_bit: send a stop bit after reading
///
///		i2c.writeto(add, buf)						# addr: slave address
///													# buf: bytearray object
///
///		i2c.readfrom_mem(addr, memadr,nbytes)		# addr: slave address
///													# memadr: memory address you want to read from
///													# number of bytes you want to read.
///
///		i2c.readfrom_mem_into(addr, memadr, buf)	# same as the above method but stores the read value in
///													# passed bytearray buffer
///
///		i2c.writeto_mem(addr, memadr, buf)			# addr: slave address
///													# memadr: memory address you want to write into
///													# byte string/buffer to write
///


static bool is_twi0_active  = false;
static bool is_twi1_active	= false;


struct twi_interface twi0, twi1;
twi_options_t twi_option;

//----Helper functions---//

// return value from status register
uint32_t get_status(Twi *ptwi) {
	return ptwi->TWI_SR;
}

void twi_stop(Twi *ptwi) {
	ptwi->TWI_CR = TWI_CR_STOP;
}

uint8_t twi_byte_sent(Twi *ptwi) {
	 return ((ptwi->TWI_SR & TWI_SR_TXRDY) == TWI_SR_TXRDY);
}

void twi_start_read(Twi *ptwi, uint8_t address,uint32_t iaddress, uint32_t isize) {

	ptwi->TWI_MMR = 0;
	ptwi->TWI_MMR = TWI_MMR_MREAD | TWI_MMR_DADR(address) | 
					((isize << TWI_MMR_IADRSZ_Pos) & TWI_MMR_IADRSZ_Msk);
	ptwi->TWI_IADR = 0;
	ptwi->TWI_IADR = iaddress;
	ptwi->TWI_CR = TWI_CR_START;
}


void twi_start_write(Twi *ptwi, uint8_t address, uint32_t iaddress, uint8_t isize, uint8_t byte) {
	
	ptwi->TWI_MMR = 0;
	ptwi->TWI_MMR = TWI_MMR_DADR(address) | ((isize << TWI_MMR_IADRSZ_Pos) & TWI_MMR_IADRSZ_Msk);

	ptwi->TWI_IADR = 0;
	ptwi->TWI_IADR = iaddress;

	twi_write_byte(ptwi, byte);

}

bool twi_wait_byte_sent( Twi *ptwi, uint32_t timeout) {
	uint32_t status = 0;
	while ((status & TWI_SR_TXRDY) != TWI_SR_TXRDY) {
		status = get_status(ptwi);

		if(status & TWI_SR_NACK) {
			return false;
		}

		if(!timeout--) {
			return false;
		}
	}
	return true;
}

// wait until byte transfer completes
 bool twi_wait_transfer(Twi *ptwi, uint32_t timeout) {
	uint32_t status = 0;

	while((status & TWI_SR_TXCOMP) != TWI_SR_TXCOMP){
		status = get_status(ptwi);

		if(status & TWI_SR_NACK) {
			return false;
		}
		if(!timeout--) {
			return false;
		}
	}
	return true;
}


 bool twi_wait_received(Twi *ptwi, uint32_t timeout) {
	uint32_t status = 0;

	while((status & TWI_SR_RXRDY) != TWI_SR_RXRDY){
		status = get_status(ptwi);

		if(status & TWI_SR_NACK){
			return false;
		}

		if(!timeout--){
			return false;
		}
	}
	return true;
}

void send_stop_bit(Twi *ptwi) {

	ptwi->TWI_CR |= TWI_CR_STOP;
}

void twi_write(Twi *ptwi, uint8_t byte) {

	if(is_twi0_active)
		twi0.tx_buffer[twi0.tx_buff_len++] = byte;

	else if(is_twi1_active)
		twi1.tx_buffer[twi1.tx_buff_len++] = byte;
}


void twi_ini_bus(Twi *ptwi) {

	if(ptwi == TWI0) {
		pmc_enable_periph_clk(ID_TWI0);
		pio_configure(
			g_APinDescription[I2C_SDA0].pPort,
			g_APinDescription[I2C_SDA0].ulPinType,
			g_APinDescription[I2C_SDA0].cpu_pin,
			g_APinDescription[I2C_SDA0].ulPinConfiguration
			);
		pio_configure(
			g_APinDescription[I2C_SCL0].pPort,
			g_APinDescription[I2C_SCL0].ulPinType,
			g_APinDescription[I2C_SCL0].cpu_pin,
			g_APinDescription[I2C_SCL0].ulPinConfiguration
			);
	}
	else if( ptwi == TWI1) {
	pio_configure(
			g_APinDescription[I2C_SDA1].pPort,
			g_APinDescription[I2C_SDA1].ulPinType,
			g_APinDescription[I2C_SDA1].cpu_pin,
			g_APinDescription[I2C_SDA1].ulPinConfiguration
			);
	pio_configure(
			g_APinDescription[I2C_SCL1].pPort,
			g_APinDescription[I2C_SCL1].ulPinType,
			g_APinDescription[I2C_SCL1].cpu_pin,
			g_APinDescription[I2C_SCL1].ulPinConfiguration
			);
	}
}


//-----------Master related functions------------//
uint32_t twi_setup_master(Twi *ptwi, uint32_t speed, uint8_t address) {
	memset((void *)&twi_option,0, sizeof(twi_option));
	twi_option.speed = speed;
	twi_option.chip = address;
	twi_option.master_clk = sysclk_get_peripheral_hz();
	twi_option.smbus = 0;

//start pmc clock for TWI0 and TWI1
if(ptwi == TWI0){
	twi_ini_bus(ptwi);
	memset((void *)&twi0, 0, sizeof(twi0));
	twi0.tx_index = 	0;
	twi0.tx_buff_len = 	0;

	is_twi0_active  = true;
	}

else if (ptwi == TWI1) {

	twi_ini_bus(ptwi);
	memset((void *)&twi1, 0, sizeof(twi1));
	twi1.tx_index =		0;
	twi1.tx_buff_len =	0;

	is_twi1_active = true;
	}

return(twi_master_init(ptwi, &twi_option));
}

uint8_t twi_readto(Twi *ptwi, uint8_t address, uint8_t quantity, uint8_t *data, bool stop) {

	int read = 0;
	twi_start_read(ptwi, address, 0, 0);
	do {

		if(read + 1 == quantity)
			send_stop_bit(ptwi);

		if(twi_wait_received(ptwi, TIMEOUT)){
			if(ptwi == TWI0)
				data[read++] = twi_read_byte(ptwi);
			else 
				data[read++] = twi_read_byte(ptwi);
			}
		else
			break;
	
	}while(read < quantity);
	twi_wait_transfer(ptwi, TIMEOUT);

	return read;
}

uint8_t twi_writeto(Twi *ptwi, 
	uint8_t txaddress, 
	uint32_t iaddress, 
	uint8_t isize, 
	const uint8_t *data,
	uint8_t quantity){

	bool interface = (ptwi == TWI0);

	//Write Data into tx buffer
	if(interface) {

		for(size_t i=0; i<quantity; i++){
			twi0.tx_buffer[twi0.tx_buff_len++] = data[i];
		}
		uint8_t error = 0;
		twi_start_write(ptwi, txaddress,0, 0, twi0.tx_buffer[0]);

		if(!twi_wait_byte_sent(ptwi, TIMEOUT)) {
			error =  TWI_RECEIVE_NACK;
		}

		if(error == 0) {
			uint16_t sent = 1;

			while(sent < twi0.tx_buff_len) {
				twi_write_byte(ptwi, twi0.tx_buffer[sent++]);
				if(! twi_wait_byte_sent(ptwi, TIMEOUT)) {
					error = TWI_ERROR_TIMEOUT;
				}
			}
		}

		if(error == 0 ){
			twi_stop(ptwi);
			if(!twi_wait_transfer(ptwi, TIMEOUT)) {
				error = TWI_SEND_OVERRUN;
			}
		}

		twi0.tx_buff_len = 0;
		return error;
	}
	else {
		
		for(size_t i=0; i<quantity; i++){
			twi0.tx_buffer[twi1.tx_buff_len++] = data[i];
		}
		uint8_t error = 0;
		twi_start_write(ptwi, txaddress,0, 0, twi1.tx_buffer[0]);

		if(!twi_wait_byte_sent(ptwi, TIMEOUT))
			error =  TWI_RECEIVE_NACK;

		if(error == 0) {
			uint16_t sent = 1;

			while(sent < twi1.tx_buff_len) {
				twi_write_byte(ptwi, twi1.tx_buffer[sent++]);
				if(! twi_wait_byte_sent(ptwi, TIMEOUT))
					error = TWI_ERROR_TIMEOUT;
			}
		}

		if(error == 0 ){
			twi_stop(ptwi);
			if(!twi_wait_transfer(ptwi, TIMEOUT))
				error = TWI_SEND_OVERRUN;
		}
		twi1.tx_buff_len = 0;
		return error;
	}
}
//-------------------------------------------------------//
//Micropython bindings

typedef struct _pyb_i2c_master_obj {
	mp_obj_base_t base;
	uint32_t baudrate;
	uint8_t address;
	uint8_t bus_id;
}pyb_i2c_master_obj;


STATIC mp_obj_t pyb_i2c_scan(mp_obj_t self_in) {
	pyb_i2c_master_obj *pin = self_in;
	mp_obj_t list = mp_obj_new_list(0, NULL);
	if(pin->bus_id ==1){
		if(is_twi0_active){
			for(int i=0x08; i<0x78; i++){
				twi_write(TWI0,0);
				twi_write(TWI0,0);
				if(twi_writeto(TWI0, i,0, 0, NULL, 0) == TWI_SUCCESS)
					mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(i));
			}
		}
	}
	if(pin->bus_id ==2){
		if(is_twi1_active){
			for(int i=0x08; i<0x78; i++){
				twi_write(TWI0,0);
				twi_write(TWI0,0);
				if(twi_writeto(TWI0, i,0, 0, NULL, 0) == TWI_SUCCESS)
					mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(i));
			}
		}
	}
	return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(i2c_scan_obj, pyb_i2c_scan);

STATIC mp_obj_t pyb_i2c_readfrom(size_t n_args, const mp_obj_t *args) {

	pyb_i2c_master_obj *i2c = MP_OBJ_TO_PTR(args[0]);
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(args[2]));

    if(i2c->bus_id == 1){
    	uint8_t ret = twi_readto(TWI0, (uint8_t)mp_obj_get_int(args[1]), vstr.len, (uint8_t *)vstr.buf, mp_obj_is_true(args[3]));
    	if(ret < 0) 
    		mp_raise_OSError(-ret);
    }
    else if(i2c->bus_id == 2){
    	uint8_t ret = twi_readto(TWI1,(uint8_t)mp_obj_get_int(args[1]), vstr.len, (uint8_t *)vstr.buf, mp_obj_is_true(args[3]));
    	if(ret < 0) 
    		mp_raise_OSError(-ret);
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(i2c_readrom, 4, pyb_i2c_readfrom);


STATIC mp_obj_t pyb_i2c_readfrom_into(size_t n_args, const mp_obj_t *args) {
	pyb_i2c_master_obj *i2c = MP_OBJ_TO_PTR(args[0]);
	mp_int_t addr = mp_obj_get_int(args[1]);
	mp_buffer_info_t bufinfo;
	uint8_t ret=0;

	mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_WRITE);

	    if(i2c->bus_id == 1){
    	ret = twi_readto(TWI0, addr, bufinfo.len, bufinfo.buf, mp_obj_is_true(args[3]));
    }
    else if(i2c->bus_id == 2){
    	 ret = twi_readto(TWI1, addr, bufinfo.len, bufinfo.buf, mp_obj_is_true(args[3]));
    }
	
	if(ret < 0) 
   		mp_raise_OSError(-ret);

    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(i2c_readrom_into, 4, pyb_i2c_readfrom_into);


STATIC mp_obj_t pyb_i2c_writeto(size_t n_args, const mp_obj_t *args) {
	pyb_i2c_master_obj *i2c = MP_OBJ_TO_PTR(args[0]);
	uint8_t addr = (uint8_t)mp_obj_get_int(args[1]);
	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
	uint8_t ret=0;

	if (i2c->bus_id == 1){
    	ret = twi_writeto(TWI0, addr, 0,0, bufinfo.buf, bufinfo.len);
    }
    else if(i2c->bus_id == 2){

		ret = twi_writeto(TWI1, addr, 0,0, bufinfo.buf, bufinfo.len);
    }
    if(ret != 0)
    	mp_raise_OSError(-ret);

    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(i2c_writeto, 3, pyb_i2c_writeto);


STATIC mp_obj_t pyb_i2c_readfrom_mem(size_t n_args, const mp_obj_t *args){
	pyb_i2c_master_obj *i2c = MP_OBJ_TO_PTR(args[0]);
	uint8_t addr = (uint8_t)mp_obj_get_int(args[1]);
	uint8_t mem = (uint8_t)mp_obj_get_int(args[2]);
	vstr_t vstr;
	uint8_t ret=0;
	vstr_init_len(&vstr, mp_obj_get_int(args[3]));

	if(i2c->bus_id ==1 ){
		twi_write(TWI0,mem >> 8);
		twi_write(TWI0,mem);
		twi_writeto(TWI0, addr, 0, 0, NULL, 0);
		ret = twi_readto(TWI0, addr, vstr.len, (uint8_t*)vstr.buf, true);
	}
	else if(i2c->bus_id == 2) {
		twi_write(TWI1, mem >> 8);
		twi_write(TWI1, mem);
		twi_writeto(TWI1, addr, 0, 0, NULL, 0);
		ret = twi_readto(TWI1, addr, vstr.len, (uint8_t*)vstr.buf, true);
	}
	if(ret != vstr.len)
		mp_raise_OSError(-ret);

	return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(i2c_readfrom_mem, 4, pyb_i2c_readfrom_mem);


void i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	pyb_i2c_master_obj *self  = self_in;

	bool active=false;
	if(self->bus_id == 1 ) 
		active = is_twi0_active;
	else
		active = is_twi1_active;

	mp_printf(print, "I2C(%u, active=%u, baudrate=%u)\n", self->bus_id, active, self->baudrate);
}

STATIC mp_obj_t pyb_i2c_readfrom_mem_into(size_t n_args, const mp_obj_t *args) {
	pyb_i2c_master_obj *i2c = MP_OBJ_TO_PTR(args[0]);
	uint8_t addr = (uint8_t)mp_obj_get_int(args[1]);
	uint8_t mem = (uint8_t)mp_obj_get_int(args[2]);
	uint8_t ret=0;

	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_WRITE);

	if(i2c->bus_id == 1) {

		twi_write(TWI0, mem >> 8);
		twi_write(TWI0, mem);
		twi_writeto(TWI0, addr, 0, 0, NULL, 0);
		ret = twi_readto(TWI0, addr, bufinfo.len, bufinfo.buf, true);
	}
	else if(i2c->bus_id == 2) {

		twi_write(TWI1, mem >> 8);
		twi_write(TWI1, mem);
		twi_writeto(TWI1, addr, 0, 0, NULL, 0);
		ret = twi_readto(TWI1, addr, bufinfo.len, bufinfo.buf, true);
	}

	if( ret != bufinfo.len)
		mp_raise_OSError(-ret);

	return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(i2c_readfrom_mem_into, 4, pyb_i2c_readfrom_mem_into);

/*

STATIC mp_obj_t write_into_tx_buffer(size_t n_args, const mp_obj_t *args) {




	return mp_const_none;
}
*/

STATIC mp_obj_t pyb_i2c_writeto_mem(size_t n_args, const mp_obj_t *args) {
	pyb_i2c_master_obj *i2c = MP_OBJ_TO_PTR(args[0]);
	uint8_t addr = (uint8_t)mp_obj_get_int(args[1]);
	uint8_t mem = (uint8_t)mp_obj_get_int(args[2]);
	uint8_t ret = 0;

	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_READ);

	if(i2c->bus_id == 1){

		twi_write(TWI0, mem >> 8);
		twi_write(TWI0, mem);
		ret = twi_writeto(TWI0, addr, 0, 0, bufinfo.buf, bufinfo.len);
	}

	else if(i2c->bus_id == 2) {
		twi_write(TWI1, mem >> 8);
		twi_write(TWI1, mem);
		twi_writeto(TWI1, addr, 0, 0, NULL, 0);
		ret  = twi_writeto(TWI1, addr, 0, 0, bufinfo.buf, bufinfo.len);
	}
	if(ret != 0)
		mp_raise_OSError(-ret);

	return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(i2c_writeto_mem, 4, pyb_i2c_writeto_mem);


STATIC mp_obj_t pin_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args){
	 mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
	 
	 int i2c_id = 0;
	 mp_obj_t i2c_pin = args[0];

	 if(MP_OBJ_IS_INT(i2c_pin)) {
	 	i2c_id = mp_obj_get_int(i2c_pin);
	 	if(i2c_id > 2)
	 		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Only two buses are available"));
	 }

	 pyb_i2c_master_obj *i2c = m_new_obj(pyb_i2c_master_obj);
	 memset(i2c, 0, sizeof(*i2c));

	 i2c->base.type = &i2c_type;
	 i2c->bus_id = i2c_id;

	 if(n_args > 1){
	 	i2c->baudrate = mp_obj_get_int(args[1]);
	 	i2c->address = mp_obj_get_int(args[2]);
	 }
	 else {
	 	i2c->baudrate = 100000;
	 	i2c->address  = 0x50;
	 }

	 //Setup I2C bus
	 if(i2c_id == 1) {
	 	if(twi_setup_master(TWI0, i2c->baudrate, i2c->address) !=TWI_SUCCESS) {
	 		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "Error occured in initialization of I2C bus 0"));
	 	}
	 }
	 else if( i2c_id == 2) {
	 	if(twi_setup_master(TWI1, i2c->baudrate, i2c->address) != TWI_SUCCESS) {
	 		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "Error occured in initialization of I2C bus 1"));
	 	}
	 }
	 
	  return i2c;
}

STATIC const mp_map_elem_t i2c_locals_dict_table[] = {

	{MP_OBJ_NEW_QSTR(MP_QSTR_scan), (mp_obj_t)&i2c_scan_obj },
	{MP_OBJ_NEW_QSTR(MP_QSTR_readfrom), (mp_obj_t)&i2c_readrom },
	{MP_OBJ_NEW_QSTR(MP_QSTR_readfrom_into), (mp_obj_t)&i2c_readrom_into},
	{MP_OBJ_NEW_QSTR(MP_QSTR_writeto), (mp_obj_t)&i2c_writeto},
	{MP_OBJ_NEW_QSTR(MP_QSTR_readfrom_mem), (mp_obj_t)&i2c_readfrom_mem},
	{MP_OBJ_NEW_QSTR(MP_QSTR_readfrom_mem_into), (mp_obj_t)&i2c_readfrom_mem_into},
	{MP_OBJ_NEW_QSTR(MP_QSTR_writeto_mem), (mp_obj_t)&i2c_writeto_mem},
	{MP_OBJ_NEW_QSTR(MP_QSTR_MASTER), MP_OBJ_NEW_SMALL_INT(MASTER)},

};

STATIC MP_DEFINE_CONST_DICT(i2c_locals_dict, i2c_locals_dict_table);

const mp_obj_type_t i2c_type = {
	{&mp_type_type},
	.name = MP_QSTR_I2C,
	.make_new = pin_make_new,
	.print = i2c_print,
	.locals_dict = (mp_obj_t)&i2c_locals_dict,
};
