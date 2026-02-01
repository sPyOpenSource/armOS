
#ifndef PIN_NAMED_DEF_H
#define PIN_NAMED_DEF_H

#include "modpinmap.h"

extern const mp_obj_type_t pin_board_pins_obj_type;
extern const mp_obj_dict_t pin_board_pins_locals_dict;
const pyb_pin_obj *pin_find(mp_obj_t user_obj);
const pyb_pin_obj *pin_find_named_pin(const mp_obj_dict_t *named_pins, mp_obj_t name);


#endif
