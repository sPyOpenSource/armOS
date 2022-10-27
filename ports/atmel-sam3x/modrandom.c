
#include "py/headers/binary.h"
#include "py/headers/runtime.h"
#include "py/headers/obj.h"
#include "py/headers/nlr.h"
#include "modrandom.h"

static volatile uint32_t random_num;

#ifdef ENABLE_TRNG

uint32_t get_random_number() {
	uint32_t status;

	status = trng_get_interrupt_status(TRNG);
	if((status & TRNG_ISR_DATRDY) == TRNG_ISR_DATRDY) {
		random_num = trng_read_output_data(TRNG);
	}
	return random_num;
}

void init_trng() {


	pmc_enable_periph_clk(ID_TRNG);
	trng_enable(TRNG);

	//Note: For some unknown reason, enabling TRNG interrupt disables the USB enumeration.

	//NVIC_DisableIRQ(TRNG_IRQn);
	//NVIC_ClearPendingIRQ(TRNG_IRQn);
	//NVIC_SetPriority(TRNG_IRQn, 0);
	//NVIC_EnableIRQ(TRNG_IRQn);
	//trng_enable_interrupt(TRNG);

}

STATIC mp_obj_t random_number(void) {

	return mp_obj_new_int(get_random_number() >> 2);
}
MP_DEFINE_CONST_FUN_OBJ_0(pyb_random, random_number);

#endif
