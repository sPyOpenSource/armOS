#include <asf.h>
#include <string.h>

#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"
#include "modpinmap.h"
#include "daac.h"


//
// class DAC: 		Digital to Analog Converter
//
//	
//	Arduino has True 12 bit DAC on pin DAC0 and DAC1
//
//
//	from machine import DAC
//
//	dac = DAC("DAC0")			# Initialize DAC on pin DAC0
//								# Default resolution is 12 bit
//								# value goes from 0 to 4095
//
//	Available Methods:
//
//	dac.write(value)			# value: write value to DAC
//
//
//


uint32_t _writeResolution = 12;

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to) {
	if (from == to) {
		return value;
	}
	if (from > to) {
		return value >> (from - to);
	}
	else {
		return value << (to - from);
	}
}

void  analog_write(uint32_t board_pin, uint32_t value) {

 uint32_t attr = g_APinDescription[board_pin].ulPinAttribute;

	if ((attr & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG) {

		EAnalogChannel channel =  g_APinDescription[board_pin].ulADCChannelNumber;

		if(channel == DA0 || channel == DA1) {
			uint32_t chDAAC = ((channel == DA0) ? 0 : 1);

			if(dacc_get_channel_status(DACC_INTERFACE) == 0) {

				pmc_enable_periph_clk(DACC_INTERFACE_ID);

				dacc_reset(DACC_INTERFACE);

				dacc_set_transfer_mode(DACC_INTERFACE, 0);

				dacc_set_power_save(DACC_INTERFACE, 0, 0);

				dacc_set_timing(DACC_INTERFACE, 0x08, 0, 0x10);

				dacc_set_analog_control(DACC_INTERFACE, DACC_ACR_IBCTLCH0(0x02) | 
														DACC_ACR_IBCTLCH1(0x02) |
														DACC_ACR_IBCTLDACCORE(0x1));

				dacc_set_channel_selection(DACC_INTERFACE, chDAAC);

				if ((dacc_get_channel_status(DACC_INTERFACE) & (1 << chDAAC)) == 0) {
				dacc_enable_channel(DACC_INTERFACE, chDAAC);
			}

			value = mapResolution(value, _writeResolution, DACC_RESOLUTION);
			dacc_write_conversion_data(DACC_INTERFACE, value);
			while((dacc_get_interrupt_status(DACC_INTERFACE) & DACC_ISR_EOC) == 0);
			return;

		}
	} 

	}
}


//-------------------------------------------------------//
//Micropython bindings

typedef struct _pyb_dac_obj {
	mp_obj_base_t base;
	uint32_t channel;
	uint8_t resol;
}pyb_dac_obj;


STATIC mp_obj_t pin_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
	mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

	mp_obj_t pin_obj = args[0];
	uint32_t channel;
	uint8_t resolution = 12;

	if( n_args > 1 ) {
		resolution = mp_obj_get_int(args[1]);
	}

	if(MP_OBJ_IS_INT(pin_obj)) {
		channel = mp_obj_get_int(pin_obj);
	} else {
		const pyb_pin_obj *pin = pin_find(pin_obj);
		if(pin->board_pin != DAC0 || pin->board_pin != DAC1) {
			nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "pin does not have DAC capabilities"));
		}

		channel = pin->board_pin;
	}
	
	pyb_dac_obj *dac = m_new_obj(pyb_dac_obj);
	memset(dac, 0, sizeof(*dac));
	dac->base.type = &dac_type;
	dac->channel = channel;
	dac->resol = resolution;
	return dac;
}

STATIC mp_obj_t dac_write(mp_obj_t self_in, mp_obj_t value) {

	pyb_dac_obj *self = self_in;
	_writeResolution = self->resol;
	analog_write(self->channel, mp_obj_get_int(value));
	return mp_const_none;
}


STATIC MP_DEFINE_CONST_FUN_OBJ_2(dac_write_obj, dac_write);


STATIC const mp_map_elem_t dac_locals_dict_table[] = {
	{MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_DAC)},
	{MP_OBJ_NEW_QSTR(MP_QSTR_write), (mp_obj_t)&dac_write_obj},
};


STATIC MP_DEFINE_CONST_DICT(dac_locals_dict, dac_locals_dict_table);

const mp_obj_type_t dac_type = {
	{&mp_type_type},
	.name = MP_QSTR_DAC,
	.make_new = pin_make_new,
	.locals_dict = (mp_obj_t)&dac_locals_dict,
};