/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include "py/headers/obj.h"
#include "py/headers/objmodule.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

/// \module os - basic "operating system" services
///
/// The `os` module contains functions for filesystem access.

STATIC const mp_map_elem_t os_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_uos) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_chdir), (mp_obj_t)&mp_vfs_chdir_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_getcwd), (mp_obj_t)&mp_vfs_getcwd_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_listdir), (mp_obj_t)&mp_vfs_listdir_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mkdir), (mp_obj_t)&mp_vfs_mkdir_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_remove), (mp_obj_t)&mp_vfs_remove_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_rename),(mp_obj_t)&mp_vfs_rename_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_rmdir), (mp_obj_t)&mp_vfs_rmdir_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_stat), (mp_obj_t)&mp_vfs_stat_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_statvfs), (mp_obj_t)&mp_vfs_statvfs_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_unlink), (mp_obj_t)&mp_vfs_remove_obj },

    { MP_OBJ_NEW_QSTR(MP_QSTR_sep), MP_OBJ_NEW_QSTR(MP_QSTR__slash_) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_mount), (mp_obj_t)&mp_vfs_mount_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_umount), (mp_obj_t)&mp_vfs_umount_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_VfsFat), (mp_obj_t)&mp_fat_vfs_type },
};

STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

const mp_obj_module_t mp_module_uos = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&os_module_globals,
};
