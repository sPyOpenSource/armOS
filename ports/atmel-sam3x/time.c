#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"
#include "due_mphal.h"
#include "asf.h"
#include "delay.h"


// module time:
//
//  Available methods:
//
//  millis()               # Returns millisecond passed from start of the program till now
//
//
//  sleep_ms()             # Delay for X milliseconds
//
//  sleep_us()              # Delay for X microseconds
//
//
//


static volatile uint32_t g_ul_ms_ticks = 0;
static bool is_systick = false;
void SysTick_Handler(void) {
    g_ul_ms_ticks++;
}

void init_systick(uint32_t divider) {
    if(SysTick_Config(sysclk_get_cpu_hz()/ divider)){
        printf("Systick config failed\n");
        is_systick = false;
    }
    is_systick = true;
}

uint32_t time_millis(void) {
    return g_ul_ms_ticks;
}

typedef struct _delay_obj {

mp_obj_base_t base;
}delay_obj;

STATIC mp_obj_t millis(void) {
    if(is_systick){
        return MP_OBJ_NEW_SMALL_INT(time_millis());
    }
    else {
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pyb_millis, millis);

STATIC mp_obj_t sleep_ms(mp_obj_t ms_obj) {
        uint32_t ms;
        ms = mp_obj_get_int(ms_obj);
        delay_ms(ms);
        return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_sleep_ms, sleep_ms);


STATIC mp_obj_t sleep_us(mp_obj_t ms_obj) {
    delay_obj  *self = ms_obj;
    (void)self;
    uint16_t us = (uint16_t)mp_obj_get_int(ms_obj);
    delay_us(us);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_sleep_us, sleep_us);

STATIC const mp_map_elem_t led_locals_dict_table[] = {

    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_time)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_sleep_ms), (mp_obj_t)&pyb_sleep_ms },
    {MP_OBJ_NEW_QSTR(MP_QSTR_sleep_us), (mp_obj_t)&pyb_sleep_us },
    {MP_OBJ_NEW_QSTR(MP_QSTR_millis), (mp_obj_t)&pyb_millis},
};

STATIC MP_DEFINE_CONST_DICT(led_locals_dict, led_locals_dict_table);

const mp_obj_type_t pyb_time_type = {
    {&mp_type_type},
//  .name = MP_QSTR_time,
    .locals_dict = (mp_obj_t)&led_locals_dict,
};
