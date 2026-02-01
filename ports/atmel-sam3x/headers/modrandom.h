
#ifndef RANDOM_H
#define RANDOM_H

#include "due_mphal.h"

void init_trng();
uint32_t get_random_number();

MP_DECLARE_CONST_FUN_OBJ_0(pyb_random);

#endif