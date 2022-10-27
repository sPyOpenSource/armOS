#include <string.h>

#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"

#include "modpwm.h"
#include "modpinmap.h"
#include "modpin.h"
#include "pwmc.h"


//
// class PWM
//
// class provides PWM on Pins 6,7,8,9 
//
//
//
//	from machine import I2C
//
//	pwm = PWM(6)								# create PWM object on pin 6	
//
//	pwm = PWM(6, freq=50, duty=255)				# create PWM object on pin 6 with frequency of 50Hz and duty cycle of 255 
//
//
//	Available methods:
//
//	pwm.resolution(16)							# set PWM resolution (default is 8)
//												# 16 is the MAX resolution possible on PWM pins : 6,7,8,9
//												# by setting resolution to 16 you get the range of 0 to 65535
//
//
//
//	pwm.duty(duty)								# set duty cycle for the current pwm object
//												# MAX value of duty cycle depends on the set resolution
//												# for 8 bit resolution duty cycle: 0 to 255
//												# for 12 bit resolution duty cycle: 0 to 4095
//												# for 16 bit resolution duty cycle: 0 to 65535
//												
//
//
//	pwm.deinit()								# set pin as OUTPUT and pull it to LOW.
//
//
//
//	TODO:	Allow to set different PWM frequencies for different pins.
//			Add Timer Pins.
//	
//
//


static uint8_t resolution = 8;
static uint16_t count =  255;
static uint32_t pwm_freq = 1000; // default 
static uint8_t PWMEnabed = false;


uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to) {
    if (from == to) {
        return value;
    }
    if (from > to) {
        return value >> (from-to);
    } else {
        return value << (to-from);
    }
}

void set_duty_cycle(uint8_t pin, uint32_t duty)	{

	if(pin  >= 6 && pin <=9) {
		duty = mapResolution(duty, resolution, PWM_RESOLUTION);
		uint32_t channel =  g_APinDescription[pin].ulPWMChannel;
		PWMC_SetDutyCycle(PWM_INTERFACE, channel, duty);
	}
}

void pwm_deinit(uint8_t pin) {
	ini_pin(pin, OUTPUT);
	set_pin(pin, LOW);
}


void setup_pwm(uint8_t pin, uint32_t freq) {


	if( pin >=6 && pin <=9) {
		uint32_t  pwm_duty = 0; 
		pwm_freq = freq;
		uint32_t channel = g_APinDescription[pin].ulPWMChannel;
		pwm_duty = mapResolution(pwm_duty, resolution, PWM_RESOLUTION);


		if(!PWMEnabed){
		pmc_enable_periph_clk(PWM_INTERFACE_ID);
		PWMC_ConfigureClocks((pwm_freq*count), 0, 84000000);
		PWMEnabed = true;
	}

		pio_configure(g_APinDescription[pin].pPort,
					  g_APinDescription[pin].ulPinType,
					  g_APinDescription[pin].cpu_pin,
					  g_APinDescription[pin].ulPinConfiguration
					  );

		PWMC_ConfigureChannel(PWM_INTERFACE, channel, PWM_CMR_CPRE_CLKA, 0, 0);
		PWMC_SetPeriod(PWM_INTERFACE, channel, count);
		PWMC_SetDutyCycle(PWM_INTERFACE, channel, pwm_duty);
		PWMC_EnableChannel(PWM_INTERFACE, channel);
		g_pinStatus[pin] = (g_pinStatus[pin] & 0xF0) | PIN_STATUS_PWM;
	}

}

/**********************************************************************/
// Micropython bindings


typedef struct _pyb_pwm_obj {
	mp_obj_base_t base;
	uint32_t channel;
	uint32_t freq;
	uint16_t duty;
	uint8_t pin;
	bool is_active;
}pyb_pwm_obj;




STATIC void pyb_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	pyb_pwm_obj *self = MP_OBJ_TO_PTR(self_in);
	mp_printf(print, "PWM(%u, freq=%u, active=%u)",self->pin, self->freq, self->is_active);
}

STATIC mp_obj_t pyb_pwm_deinit(mp_obj_t self_in) {
	pyb_pwm_obj *self = self_in;
	pwm_deinit(self->pin);
	self->is_active = false;
	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_deinit_obj, pyb_pwm_deinit);


STATIC mp_obj_t pyb_set_duty_cycle(size_t n_args, const mp_obj_t *args)	{

	pyb_pwm_obj *self = MP_OBJ_TO_PTR(args[0]);
	if(n_args == 1) {
		return MP_OBJ_NEW_SMALL_INT(self->freq);
	} else {
		set_duty_cycle(self->pin, mp_obj_get_int(args[1]));
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_duty_obj, 1, 2, pyb_set_duty_cycle);


STATIC mp_obj_t pyb_set_resolution(size_t n_args, const mp_obj_t *args) {
	pyb_pwm_obj *self = MP_OBJ_TO_PTR(args[0]);
	uint8_t resol = (uint8_t)mp_obj_get_int(args[1]);
	if(resol > 16)
		resol  = 16;

	resolution = resol;
	setup_pwm(self->pin, self->freq);
	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(pwm_set_resolution, 2, pyb_set_resolution);

STATIC void pyb_pwm_init_helper(pyb_pwm_obj *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args){
	enum { ARG_freq=0, ARG_duty=1};
	static const mp_arg_t allowed_args[] = {
 		{ MP_QSTR_freq, MP_ARG_INT, {.u_int = -1} },
 		{ MP_QSTR_duty, MP_ARG_INT, {.u_int = -1} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self_in->is_active = true;
    self_in->freq = pwm_freq;

    if(args[ARG_freq].u_int != -1){
    	self_in->freq = args[ARG_freq].u_int;
    }

    setup_pwm(self_in->pin, self_in->freq);
	
	if(args[ARG_duty].u_int != -1){
    	set_duty_cycle(self_in->pin, args[ARG_duty].u_int);
    	self_in->duty = args[ARG_duty].u_int;
    }

}

STATIC mp_obj_t pin_make_new (const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
	mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

	int pwm_pin = 0;
	mp_obj_t pwm_obj = args[0];

	if(MP_OBJ_IS_INT(pwm_obj)){

		pwm_pin = mp_obj_get_int(pwm_obj);
		if(pwm_pin <6 || pwm_pin > 9 ){
			nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Currently PWM is available only on pins: 6,7,8,9"));
		} else {

			pyb_pwm_obj *pwm = m_new_obj(pyb_pwm_obj);
			memset(pwm, 0, sizeof(*pwm));
			pwm->base.type = &pwm_type;
			pwm->pin = pwm_pin;
			pwm->channel =  g_APinDescription[pwm_pin].ulPWMChannel;
			pwm->is_active = false;
			pwm->duty = 0;

			mp_map_t kw_args;
			mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
			pyb_pwm_init_helper(pwm,  n_args - 1, args + 1, &kw_args);

			return MP_OBJ_FROM_PTR(pwm);
		}
	} else {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Integer value is expected."));
	}
}

STATIC const mp_rom_map_elem_t pwm_locals_dict_table[] = {

	{MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_pwm_deinit_obj)},
	{MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&pyb_pwm_duty_obj)},
	{MP_ROM_QSTR(MP_QSTR_resolution), MP_ROM_PTR(&pwm_set_resolution)},
};

STATIC MP_DEFINE_CONST_DICT(pwm_locals_dict, pwm_locals_dict_table);

const mp_obj_type_t pwm_type = {

	{&mp_type_type},
	.name = MP_QSTR_PWM,
	.print = pyb_pwm_print,
	.make_new = pin_make_new,
	.locals_dict =  (mp_obj_dict_t*)&pwm_locals_dict,
};
