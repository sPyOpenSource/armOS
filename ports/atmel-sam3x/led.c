
#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"
#include "due_mphal.h"
#include "modpyb.h"


//	class LED
//
//	LED object controls the LED present on pin number 13 
//
// Available methods:
//
//	LED.off()		# Tur off the LED on board
//
//	LED.on()		# Turn on the LED on board
//
//	LED.toggle() 	# Toggle LED pin.
//
//

void led_init()	{
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB27, PIO_DEFAULT);

}


typedef struct _pyb_led_obj_t {
	mp_obj_base_t base;
}pyb_led_obj_t;


STATIC const pyb_led_obj_t pyb_led_obj[] =	{
	  {{&pyb_led_type}},
};


#define LED_ID(obj) ((obj) - &pyb_led_obj[0] + 1)


void pyb_led_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_led_obj_t *self = self_in;
    mp_printf(print, "LED(%u)", LED_ID(self));
}


mp_obj_t led_obj_on(void) {
	PIOB->PIO_SODR = PIO_PB27;
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(led_obj_on_obj, led_obj_on);


mp_obj_t led_obj_off(void) {
	if(PIOB->PIO_ODSR & PIO_PB27)
		PIOB->PIO_CODR  = PIO_PB27;

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(led_obj_off_obj, led_obj_off);


mp_obj_t led_obj_toggle(void) {

	if(PIOB->PIO_ODSR & PIO_PB27)
		PIOB->PIO_CODR  = PIO_PB27;
	else
		PIOB->PIO_SODR = PIO_PB27;

	return mp_const_none;

}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(led_obj_toggle_obj, led_obj_toggle);


STATIC const mp_map_elem_t led_locals_dict_table[] = {

	{ MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_LED)},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_on), (mp_obj_t)&led_obj_on_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&led_obj_off_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_toggle), (mp_obj_t)&led_obj_toggle_obj },
};

STATIC MP_DEFINE_CONST_DICT(led_locals_dict, led_locals_dict_table);

const mp_obj_type_t pyb_led_type = {
	{&mp_type_type},
	.name = MP_QSTR_LED,
	.print = pyb_led_print,
	.locals_dict = (mp_obj_t)&led_locals_dict,
};
