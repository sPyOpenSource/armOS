/*
 * machine.SDCard for the atmel-sam3x (Arduino Due) port.
 *
 * Exposes the SPI SD card as a MicroPython block device so that it can be
 * wrapped with os.VfsFat and mounted:
 *
 *     from machine import SDCard, Pin
 *     import os
 *     sd = SDCard(Pin(77))            # PA28 / NPCS0
 *     os.mount(os.VfsFat(sd), "/sd")
 *
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2026
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>

#include "py/headers/obj.h"
#include "py/headers/runtime.h"
#include "modpinmap.h"
#include "pin_named_def.h"
#include "extmod/vfs.h"
#include "sdcard_spi.h"
#include "sdcard.h"

typedef struct _pyb_sdcard_obj_t {
    mp_obj_base_t base;
} pyb_sdcard_obj_t;

// Single-card driver: sdcard_spi.c holds the one instance's configuration.
STATIC const pyb_sdcard_obj_t pyb_sdcard_obj = { { &pyb_sdcard_type } };

STATIC mp_obj_t pyb_sdcard_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 3, false);

    const pyb_pin_obj *cs_pin = pin_find(args[0]);
    uint32_t freq = (n_args >= 2) ? mp_obj_get_int(args[1]) : 25000000;
    bool use_hw_cs = (n_args >= 3) ? mp_obj_is_true(args[2]) : false;

    sdcard_config_t config;
    config.cs_pin = cs_pin->board_pin;
    config.spi_freq = freq;
    config.use_hw_cs = use_hw_cs;

    if (!sdcard_init(&config)) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "SDCard init failed"));
    }
    return (mp_obj_t)&pyb_sdcard_obj;
}

STATIC mp_obj_t pyb_sdcard_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);
    bool ok = sdcard_read_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ok);
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_readblocks_obj, pyb_sdcard_readblocks);

STATIC mp_obj_t pyb_sdcard_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);
    bool ok = sdcard_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ok);
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_writeblocks_obj, pyb_sdcard_writeblocks);

STATIC mp_obj_t pyb_sdcard_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    mp_int_t arg = mp_obj_get_int(arg_in);
    return MP_OBJ_NEW_SMALL_INT(sdcard_ioctl(cmd, arg));
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_ioctl_obj, pyb_sdcard_ioctl);

STATIC mp_obj_t pyb_sdcard_present(mp_obj_t self) {
    return mp_obj_new_bool(sdcard_is_present());
}
MP_DEFINE_CONST_FUN_OBJ_1(pyb_sdcard_present_obj, pyb_sdcard_present);

STATIC const mp_map_elem_t pyb_sdcard_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_present), (mp_obj_t)&pyb_sdcard_present_obj },
    // block device protocol
    { MP_OBJ_NEW_QSTR(MP_QSTR_readblocks), (mp_obj_t)&pyb_sdcard_readblocks_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_writeblocks), (mp_obj_t)&pyb_sdcard_writeblocks_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ioctl), (mp_obj_t)&pyb_sdcard_ioctl_obj },
};

STATIC MP_DEFINE_CONST_DICT(pyb_sdcard_locals_dict, pyb_sdcard_locals_dict_table);

const mp_obj_type_t pyb_sdcard_type = {
    { &mp_type_type },
    .name = MP_QSTR_SDCard,
    .make_new = pyb_sdcard_make_new,
    .locals_dict = (mp_obj_t)&pyb_sdcard_locals_dict,
};
