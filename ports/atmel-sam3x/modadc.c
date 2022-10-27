
#include <asf.h>
#include <string.h>

#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"
#include "modpinmap.h"
#include "modadc.h"


//
//
// class ADC - Analog to Digital Converter
//
//
//	Arduino Due has ADC support on pins A0 to A11
//
//
//	from machine import ADC
//
//
//	adc = ADC("A0")				# initialized ADC on pin 'A0'
//								# Default resolution is 12 bit
//								# value goes from 0 to 4095
//
//	Available methods:
//	
//		
//	adc.read()					# read the value of the pin	
//
//
//


static int _readResolution  = 12;
//static int _writeResolution = 12;


static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to) {
	if (from == to)
		return value;
	if (from > to)
		return value >> (from-to);
	else
		return value << (to-from);
}


eAnalogReference anlog_reference = AR_DEFAULT;

uint32_t analog_read(uint32_t board_pin) {

	uint32_t value = 0;
	uint32_t channel;

	channel = g_APinDescription[board_pin].ulADCChannelNumber;

	static uint32_t new_channel = -1;
	switch(g_APinDescription[board_pin].ulAnalogChannel) {

		case ADC0 :
		case ADC1 :
		case ADC2 :
		case ADC3 :
		case ADC4 :
		case ADC5 :
		case ADC6 :
		case ADC7 :
		case ADC8 :
		case ADC9 :
		case ADC10:
		case ADC11:

			if(adc_get_channel_status(ADC, channel)!=1) {
				adc_enable_channel( ADC, channel );
				if(new_channel != (uint32_t)-1 && channel != new_channel)
					adc_disable_channel(ADC, new_channel);
				new_channel = channel;
				g_pinStatus[board_pin] = (g_pinStatus[board_pin] & 0xF0) | PIN_STATUS_ANALOG;
			}

			adc_start(ADC);
			while((adc_get_status(ADC) & ADC_ISR_DRDY) != ADC_ISR_DRDY){}

			value = adc_get_latest_value(ADC);
			value = mapResolution(value, ADC_RESOLUTION, _readResolution);
			break;

		default:
			value = 0;
			break;

	}
	return value;

}

//----------------------------------------------------------------------------//
//Micropython Bindings


typedef struct _pyb_adc_obj {
	mp_obj_base_t base;
	uint32_t channel;

}pyb_obj_adc;

STATIC mp_obj_t pin_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
	mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);


	mp_obj_t pin_obj = args[0];
	uint32_t channel;

	if(MP_OBJ_IS_INT(pin_obj)) {
		channel = mp_obj_get_int(pin_obj);
	} else {
		const pyb_pin_obj *pin = pin_find(pin_obj);
		if(pin->board_pin < A0 || pin->board_pin > CANTX) {
			 nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "pin does not have ADC capabilities"));
		}

		channel = pin->board_pin;
	}

	pyb_obj_adc *adc = m_new_obj(pyb_obj_adc);
	memset(adc, 0, sizeof(*adc));
	adc->base.type = &adc_type;
	adc->channel = channel;
	return adc;

}

STATIC mp_obj_t adc_read(mp_obj_t self_in) {
	pyb_obj_adc *self = self_in;
	uint32_t data = analog_read(self->channel);
	return mp_obj_new_int(data);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(adc_read_obj, adc_read);

STATIC const mp_map_elem_t adc_locals_dict_table[] = {

	{MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_ADC)},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_read), (mp_obj_t)&adc_read_obj },
};

STATIC MP_DEFINE_CONST_DICT(adc_locals_dict, adc_locals_dict_table);


const mp_obj_type_t adc_type = {
	{&mp_type_type},
	.name = MP_QSTR_ADC,
	.make_new = pin_make_new,
	.locals_dict = (mp_obj_t)&adc_locals_dict,
};
