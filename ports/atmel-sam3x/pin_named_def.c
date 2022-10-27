#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"

#include "pin_named_def.h"

STATIC bool pin_class_debug = false;

const pyb_pin_obj *pin_find_named_pin(const mp_obj_dict_t *named_pins, mp_obj_t name) {
    mp_map_t *named_map = mp_obj_dict_get_map((mp_obj_t)named_pins);
    mp_map_elem_t *named_elem = mp_map_lookup(named_map, name, MP_MAP_LOOKUP);
    if (named_elem != NULL && named_elem->value != NULL) {
        return named_elem->value;
    }
    return NULL;
}


const pyb_pin_obj *pin_find(mp_obj_t user_obj) {
    const pyb_pin_obj *pin_obj;

    // If a pin was provided, then use it
    if (MP_OBJ_IS_TYPE(user_obj, &pin_type)) {
        pin_obj = user_obj;
        if (pin_class_debug) {
            printf("Pin map passed pin ");
            mp_obj_print((mp_obj_t)pin_obj, PRINT_STR);
            printf("\n");
        }
        return pin_obj;
    }

    if (MP_STATE_PORT(pin_class_mapper) != mp_const_none) {
        pin_obj = mp_call_function_1(MP_STATE_PORT(pin_class_mapper), user_obj);
        if (pin_obj != mp_const_none) {
            if (!MP_OBJ_IS_TYPE(pin_obj, &pin_type)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Pin.mapper didn't return a Pin object"));
            }
            if (pin_class_debug) {
                printf("Pin.mapper maps ");
                mp_obj_print(user_obj, PRINT_REPR);
                printf(" to ");
                mp_obj_print((mp_obj_t)pin_obj, PRINT_STR);
                printf("\n");
            }
            return pin_obj;
        }
        // The pin mapping function returned mp_const_none, fall through to
        // other lookup methods.
    }

    if (MP_STATE_PORT(pin_class_map_dict) != mp_const_none) {
        mp_map_t *pin_map_map = mp_obj_dict_get_map(MP_STATE_PORT(pin_class_map_dict));
        mp_map_elem_t *elem = mp_map_lookup(pin_map_map, user_obj, MP_MAP_LOOKUP);
        if (elem != NULL && elem->value != NULL) {
            pin_obj = elem->value;
            if (pin_class_debug) {
                printf("Pin.map_dict maps ");
                mp_obj_print(user_obj, PRINT_REPR);
                printf(" to ");
                mp_obj_print((mp_obj_t)pin_obj, PRINT_STR);
                printf("\n");
            }
            return pin_obj;
        }
    }

    // See if the pin name matches a board pin
    pin_obj = pin_find_named_pin(&pin_board_pins_locals_dict, user_obj);
    if (pin_obj) {
        if (pin_class_debug) {
            printf("Pin.board maps ");
            mp_obj_print(user_obj, PRINT_REPR);
            printf(" to ");
            mp_obj_print((mp_obj_t)pin_obj, PRINT_STR);
            printf("\n");
        }
        return pin_obj;
    }


    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "pin '%s' not a valid pin identifier", mp_obj_str_get_str(user_obj)));
}



STATIC const mp_map_elem_t pin_board_pins_locals_dict_table[] =   {
  { MP_OBJ_NEW_QSTR(MP_QSTR_A0), (mp_obj_t)&g_APinDescription[A0] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A1), (mp_obj_t)&g_APinDescription[A1] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A2), (mp_obj_t)&g_APinDescription[A2] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A3), (mp_obj_t)&g_APinDescription[A3] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A4), (mp_obj_t)&g_APinDescription[A4] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A5), (mp_obj_t)&g_APinDescription[A5] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A6), (mp_obj_t)&g_APinDescription[A6] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A7), (mp_obj_t)&g_APinDescription[A7] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A8), (mp_obj_t)&g_APinDescription[A8] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A9), (mp_obj_t)&g_APinDescription[A9] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A10), (mp_obj_t)&g_APinDescription[A10] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A11), (mp_obj_t)&g_APinDescription[A11] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_DAC0), (mp_obj_t)&g_APinDescription[DAC0] },
  { MP_OBJ_NEW_QSTR(MP_QSTR_DAC1), (mp_obj_t)&g_APinDescription[DAC1] },
 
};

MP_DEFINE_CONST_DICT(pin_board_pins_locals_dict, pin_board_pins_locals_dict_table);

const mp_obj_type_t pin_board_pins_obj_type = {
    { &mp_type_type },
    .name = MP_QSTR_board,
    .locals_dict = (mp_obj_t)&pin_board_pins_locals_dict,
};
