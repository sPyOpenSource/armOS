#include <stdio.h>

#include "py/headers/obj.h"
#include "py/headers/mphal.h"
#include "modpyb.h"
#include "modpinmap.h"
#include "asf.h"
#include "modrandom.h"

//
// module machine:
//
//	Available methods:
//
//	disable_irq()			# Disable interrupts
//
//	enable_irq()			# Enable interrupts
//
//
//	reset()					# Performs Hard reset
//
//	
//	reset_cause()			# Returns the reason of last reset
//
//							# 0 -> Power up reset
//							# 1 -> Return from backup mode
//							# 2 -> Watchdog reset
//							# 3 -> Software reset ( By calling reset() )
//
//
//	wfi()					# suspend execution and wait for an interrupt
//
//
//	random()				# returns a 30 bit True random number
//
//
//

STATIC mp_obj_t pyb_disable_irq(void) {

	__disable_irq();
	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(pyb_dsbl_irq, pyb_disable_irq);

STATIC mp_obj_t pyb_enable_irq(void) {

	__disable_irq();
	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(pyb_enble_irq, pyb_enable_irq);


STATIC mp_obj_t pyb_reset(void) {
	const int RST_KEY = 0xA5;
	RSTC->RSTC_CR = RSTC_CR_KEY(RST_KEY) | RSTC_CR_PROCRST | RSTC_CR_PERRST;
	while(true);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pyb_hard_rest, pyb_reset);

STATIC mp_obj_t reset_cause(void) {
	
	return mp_obj_new_int((rstc_get_reset_cause(RSTC)>> 8));

}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pyb_reset_cause, reset_cause);

STATIC mp_obj_t mp_wfi(void) {

	__WFI();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pyb_wfi, mp_wfi);



STATIC const mp_map_elem_t pyb_module_globals_table[]= {

	{ MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_machine)},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_LED), (mp_obj_t)&pyb_led_type  },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_Pin), (mp_obj_t)&pin_type},
	{MP_OBJ_NEW_QSTR(MP_QSTR_time), (mp_obj_t)&pyb_time_type},
	{MP_OBJ_NEW_QSTR(MP_QSTR_ADC), (mp_obj_t)&adc_type},
	{MP_OBJ_NEW_QSTR(MP_QSTR_DAC), (mp_obj_t)&dac_type},
	{MP_OBJ_NEW_QSTR(MP_QSTR_disable_irq), (mp_obj_t)&pyb_dsbl_irq},
	{MP_OBJ_NEW_QSTR(MP_QSTR_enable_irq), (mp_obj_t)&pyb_enble_irq},
	{MP_OBJ_NEW_QSTR(MP_QSTR_I2C), (mp_obj_t)&i2c_type},
	{MP_OBJ_NEW_QSTR(MP_QSTR_PWM), (mp_obj_t)&pwm_type},
	{MP_OBJ_NEW_QSTR(MP_QSTR_reset), (mp_obj_t)&pyb_hard_rest},
	{MP_OBJ_NEW_QSTR(MP_QSTR_reset_cause), (mp_obj_t)&pyb_reset_cause},
	{MP_OBJ_NEW_QSTR(MP_QSTR_wfi), (mp_obj_t)&pyb_wfi},


	#ifdef ENABLE_TRNG
	{MP_OBJ_NEW_QSTR(MP_QSTR_random), (mp_obj_t)&pyb_random},
	#endif
};


STATIC MP_DEFINE_CONST_DICT(pyb_module_globals, pyb_module_globals_table);

const mp_obj_module_t pyb_module = {
	.base =  { &mp_type_module },
	.globals = (mp_obj_dict_t*)&pyb_module_globals,
};
