
#ifndef MODPWM_H
#define MODPWM_H

extern const mp_obj_type_t pwm_type;

void set_pwm_freq(uint32_t pwm);
void set_duty_cycle(uint8_t pin, uint32_t duty);
uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to);

#endif