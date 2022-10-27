#ifndef MODPIN_H
#define MODPIN_H

void set_pin(uint32_t cpu_pin, uint32_t value);
void ini_pin(uint32_t cpu_pin, uint32_t Mode);
int get_pin(uint32_t board_pin);
uint32_t  get_mode(mp_obj_t self_in);


#endif