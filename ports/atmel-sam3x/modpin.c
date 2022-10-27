
#include <string.h>

#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"
#include "modpinmap.h"
#include "pin_named_def.h"
#include "modpin.h"


//  class Pin - control I/O pins
//  A pin is the basic object to control I/O pins.  It has methods to set
//  the mode of the pin (input, output, etc) and methods to get and set the
//  digital logic level.
//
//
//  from machine import Pin
//  
//  pin = Pin(13)           # Pass the pin number you want to use.
//                          # The pin will not be initialized unless you call init() method.
//
//  pin = Pin(13, Pin.OUT)  # Passing pin mode will initializes the pin.
//
//
// Available methods:
//
//  pin.init(Mode)          # Initialize/re-initialize pin with passed Mode
//                          # Available modes: Pin.OUT, Pin.IN, Pin.PULLUP (Equivalent to Arduino's modes)   
//       
//  pin.value()             # Returns current logic level of pin.
//                          # 1 if pin is HIGH, 0 if pin is LOW
//
//  pin.value(value)        # Set logic level of the pin.
//                          # 1 for HIGH, 0 for LOW
//
//
//  pin.id()                # Returns the pin number associated with object.
//
//
//  pin.mode()              # Returns the Mode of the pin.
//                          # 0 if pin is not initialized
//                          # 1 if pin is set as Input 
//                          # 2 if pin is set as Output
//
//
//


//same as pinMode of arduino
void ini_pin(uint32_t cpu_pin, uint32_t Mode){

    if(g_APinDescription[cpu_pin].ulPinType == PIO_NOT_A_PIN)
        return;  

    if((g_pinStatus[cpu_pin] & 0xF) == PIN_STATUS_ANALOG);
        adc_disable_channel(ADC, g_APinDescription[cpu_pin].ulADCChannelNumber);

   if((g_pinStatus[cpu_pin] & 0xF) < PIN_STATUS_DIGITAL_OUTPUT && g_pinStatus[cpu_pin] != 0){

        if (((g_pinStatus[cpu_pin] & 0xF) == PIN_STATUS_DIGITAL_INPUT && Mode == INPUT) ||
            ((g_pinStatus[cpu_pin] & 0xF) == PIN_STATUS_DIGITAL_INPUT_PULLUP && Mode == INPUT_PULLUP) ||
            ((g_pinStatus[cpu_pin] & 0xF) == PIN_STATUS_DIGITAL_OUTPUT && Mode == OUTPUT))
            return;
    }

    switch(Mode){

        case INPUT:
            pmc_enable_periph_clk(g_APinDescription[cpu_pin].ulPeripheralId);
            pio_configure(
                g_APinDescription[cpu_pin].pPort,
                PIO_INPUT,
                g_APinDescription[cpu_pin].cpu_pin,
                0 );
            g_pinStatus[cpu_pin] = (g_pinStatus[cpu_pin] & 0xF) | PIN_STATUS_DIGITAL_INPUT;
            g_pinMode[cpu_pin] = Mode;
        break;

        case INPUT_PULLUP:          
            pmc_enable_periph_clk( g_APinDescription[cpu_pin].ulPeripheralId ) ;
            pio_configure(
            g_APinDescription[cpu_pin].pPort,
            PIO_INPUT,
            g_APinDescription[cpu_pin].cpu_pin,
            PIO_PULLUP ) ;
            g_pinStatus[cpu_pin] = (g_pinStatus[cpu_pin] & 0xF0) | PIN_STATUS_DIGITAL_INPUT_PULLUP;
            g_pinMode[cpu_pin] = Mode;
        break;

        case OUTPUT:
            pio_configure(
            g_APinDescription[cpu_pin].pPort,
            (g_pinStatus[cpu_pin] & 0xF0) >> 4 ? PIO_OUTPUT_1 : PIO_OUTPUT_0,
            g_APinDescription[cpu_pin].cpu_pin,
            g_APinDescription[cpu_pin].ulPinConfiguration ) ;
            g_pinStatus[cpu_pin] = (g_pinStatus[cpu_pin] & 0xF0) | PIN_STATUS_DIGITAL_OUTPUT;
            g_pinMode[cpu_pin] = Mode;

            /* if all pins are output, disable PIO Controller clocking, reduce power consumption */
            if ( g_APinDescription[cpu_pin].pPort->PIO_OSR == 0xffffffff )
            {
                pmc_disable_periph_clk( g_APinDescription[cpu_pin].ulPeripheralId ) ;
            }
        break ;

        default:
        break;
    }
}


// same as digital_write
// set the logic level of Pin
void set_pin(uint32_t cpu_pin, uint32_t value) {

    if(g_APinDescription[cpu_pin].ulPinType == PIO_NOT_A_PIN)
        return;

    if((g_pinStatus[cpu_pin] & 0xF) == PIN_STATUS_PWM)
        ini_pin(cpu_pin, OUTPUT);

    g_pinStatus[cpu_pin] = (g_pinStatus[cpu_pin] & 0x0F) | (value << 4);

    if(pio_get_output_data_status( g_APinDescription[cpu_pin].pPort, g_APinDescription[cpu_pin].cpu_pin ) == 0)
        pio_pull_up( g_APinDescription[cpu_pin].pPort, g_APinDescription[cpu_pin].cpu_pin, value );

    else
        pio_set_output( g_APinDescription[cpu_pin].pPort, g_APinDescription[cpu_pin].cpu_pin, value, 0, PIO_PULLUP ) ;
}


// returns the logic level of the Pin
int get_pin(uint32_t board_pin) {

    if((g_pinStatus[board_pin] & 0xF) == PIN_STATUS_DIGITAL_OUTPUT)
        return (g_pinStatus[board_pin] & 0xF0 ) >> 4;


    if((g_pinStatus[board_pin] & 0xF ) == PIO_NOT_A_PIN)
        return LOW;

    if(pio_get( g_APinDescription[board_pin].pPort, PIO_INPUT, g_APinDescription[board_pin].board_pin ) == 1)
        return HIGH;

    return LOW;
}

uint32_t  get_mode(mp_obj_t self_in) {
    pyb_pin_obj *self = self_in;
    return g_pinMode[self->board_pin];
}


//---------------------------------Micropython bindings---------------------------------------------//

void pin_init0(void) {
    MP_STATE_PORT(pin_class_mapper) = mp_const_none;
    MP_STATE_PORT(pin_class_map_dict) = mp_const_none;
    for(uint8_t i=0; i<PINS_COUNT; i++)
        g_pinMode[i]  = -1;
}



STATIC mp_obj_t pin_high(mp_obj_t self_in) {
    pyb_pin_obj *self = self_in;

    set_pin(self->board_pin, HIGH);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pin_high, pin_high);



STATIC mp_obj_t pin_low(mp_obj_t self_in) {
    pyb_pin_obj *self = self_in;

    set_pin(self->board_pin, LOW);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pin_low, pin_low);



STATIC mp_obj_t pyb_init_helper(const pyb_pin_obj *self_in, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);



STATIC mp_obj_t pyb_pin_call(mp_obj_t self_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args)  {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    PinDescription *self = self_in;

    if(n_args == 0 ){
        //get pin
        return MP_OBJ_NEW_SMALL_INT(get_pin(self->board_pin));
    } else {
       set_pin(self->board_pin, mp_obj_is_true(args[0]));            
        return mp_const_none;
    }
}


STATIC mp_obj_t pyb_pin_value(mp_uint_t n_args, const mp_obj_t *args){
    return pyb_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pin_value_obj, 1, 2, pyb_pin_value);



STATIC mp_obj_t pyb_pin_id(mp_obj_t pin) {

    pyb_pin_obj *self = pin;
    return MP_OBJ_NEW_SMALL_INT(self->board_pin);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pin_id_obj, pyb_pin_id);



STATIC mp_obj_t pin_mode(mp_obj_t self_in) {
    return MP_OBJ_NEW_SMALL_INT(get_mode(self_in));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pin_mode, pin_mode);


STATIC mp_obj_t pyb_init_helper(const pyb_pin_obj *self_in, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_mode, MP_ARG_REQUIRED | MP_ARG_INT },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const pyb_pin_obj *self = self_in;
    uint32_t Mode = args[0].u_int;
    ini_pin(self->board_pin, Mode);
    return mp_const_none;
}


STATIC mp_obj_t pyb_pin_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pin_init_obj, 1, pyb_pin_init);


STATIC mp_obj_t pin_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
	mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    uint32_t pin;

    if(MP_OBJ_IS_INT(args[0])) {
        pin = mp_obj_get_int(args[0]);
    } else {
        const pyb_pin_obj *Pin = pin_find(args[0]);
        pin = Pin->board_pin;
    }

    pyb_pin_obj *P = NULL;
    P = (pyb_pin_obj*)&g_APinDescription[pin];
		if(n_args > 1 || n_kw > 0 ) {

			mp_map_t  kw_args;
            mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
            pyb_init_helper(P,n_args-1, args+1, &kw_args);
			
		}
		return (mp_obj_t)P;
}


STATIC const mp_map_elem_t pin_locals_dict_table[] = {

    {MP_OBJ_NEW_QSTR(MP_QSTR_value),     (mp_obj_t)&pyb_pin_value_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_PULLUP),    MP_OBJ_NEW_SMALL_INT(INPUT_PULLUP)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_IN),        MP_OBJ_NEW_SMALL_INT(INPUT)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_OUT),       MP_OBJ_NEW_SMALL_INT(OUTPUT)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_id),        (mp_obj_t)&pyb_pin_id_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_board),    (mp_obj_t)&pin_board_pins_obj_type},
    {MP_OBJ_NEW_QSTR(MP_QSTR_mode),     (mp_obj_t)&pyb_pin_mode},
    {MP_OBJ_NEW_QSTR(MP_QSTR_init),     (mp_obj_t)&pin_init_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_high),     (mp_obj_t)&pyb_pin_high},
    {MP_OBJ_NEW_QSTR(MP_QSTR_low),     (mp_obj_t)&pyb_pin_low},

};

STATIC MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_dict_table);

const mp_obj_type_t pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .make_new = pin_make_new,
    .call = pyb_pin_call,
    .locals_dict = (mp_obj_t)&pin_locals_dict,
};
