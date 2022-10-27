
#ifndef TIME_H
#define TIME_H

extern const mp_obj_type_t pyb_time_type;
extern const mp_obj_module_t pyb_module;

void init_systick(uint32_t divider);
uint32_t time_millis(void);

#endif