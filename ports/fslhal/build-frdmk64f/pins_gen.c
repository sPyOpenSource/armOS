// This file was automatically generated by make-pins.py
//
// --af boards/mk64fn1M0xxx12_af.csv
// --board boards/frdmk64f/frdmk64f_pins.csv
// --prefix boards/mk20dx256_prefix.c

// stm32fxx-prefix.c becomes the initial portion of the generated pins file.

#include <stdio.h>
#include <mk20dx128.h>

#include "py/headers/obj.h"
#include "teensy_hal.h"
#include "MK64F12.h"
#include "pin.h"

#define AF(af_idx, af_fn, af_unit, af_type, af_ptr) \
{ \
    { &pin_af_type }, \
    .name = MP_QSTR_AF ## af_idx ## _ ## af_fn ## af_unit, \
    .idx = (af_idx), \
    .fn = AF_FN_ ## af_fn, \
    .unit = (af_unit), \
    .type = AF_PIN_TYPE_ ## af_fn ## _ ## af_type, \
    .af_fn = (af_ptr) \
}

#define PIN(p_port, p_pin, p_num_af, p_af, p_adc_num, p_adc_channel) \
{ \
    { &pin_type }, \
    .name = MP_QSTR_ ## p_port ## p_pin, \
    .port = PORT_ ## p_port, \
    .pin = (p_pin), \
    .num_af = (p_num_af), \
    .pin_mask = (1 << (p_pin)), \
    .gpio = GPIO ## p_port, \
    .af = p_af, \
    .adc_num = p_adc_num, \
    .adc_channel = p_adc_channel, \
}

// const pin_af_obj_t pin_Z0_af[] = {
  //( 0, ADC     ,  0, DP1       , ADC0    ), // ADC0_DP1
  //( 1, PTZ     ,  0,           , PTZ0    ), // PTZ0
// };

const pin_obj_t pin_Z0 = PIN(Z, 0, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z1_af[] = {
  //( 0, ADC     ,  0, DM1       , ADC0    ), // ADC0_DM1
  //( 1, PTZ     ,  1,           , PTZ1    ), // PTZ1
// };

const pin_obj_t pin_Z1 = PIN(Z, 1, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z2_af[] = {
  //( 0, ADC     ,  1, DP1       , ADC1    ), // ADC1_DP1
  //( 1, PTZ     ,  2,           , PTZ2    ), // PTZ2
// };

const pin_obj_t pin_Z2 = PIN(Z, 2, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z3_af[] = {
  //( 0, ADC     ,  1, DM1       , ADC1    ), // ADC1_DM1
  //( 1, PTZ     ,  3,           , PTZ3    ), // PTZ3
// };

const pin_obj_t pin_Z3 = PIN(Z, 3, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z4_af[] = {
  //( 0, ADC     ,  0, DP0
ADC1_DP3, ADC0    ), // ADC0_DP0
ADC1_DP3
  //( 1, PTZ     ,  4,           , PTZ4    ), // PTZ4
// };

const pin_obj_t pin_Z4 = PIN(Z, 4, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z5_af[] = {
  //( 0, ADC     ,  0, DM0
ADC1_DM3, ADC0    ), // ADC0_DM0
ADC1_DM3
  //( 1, PTZ     ,  5,           , PTZ5    ), // PTZ5
// };

const pin_obj_t pin_Z5 = PIN(Z, 5, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z6_af[] = {
  //( 0, ADC     ,  1, DP0
ADC0_DP3, ADC1    ), // ADC1_DP0
ADC0_DP3
  //( 1, PTZ     ,  6,           , PTZ6    ), // PTZ6
// };

const pin_obj_t pin_Z6 = PIN(Z, 6, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z7_af[] = {
  //( 0, ADC     ,  1, DM0
ADC0_DM3, ADC1    ), // ADC1_DM0
ADC0_DM3
  //( 1, PTZ     ,  7,           , PTZ7    ), // PTZ7
// };

const pin_obj_t pin_Z7 = PIN(Z, 7, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z8_af[] = {
  //( 0, VREF    ,  0, OUT
CMP1_IN5
CMP0_IN5
ADC1_SE18, VREF    ), // VREF_OUT
CMP1_IN5
CMP0_IN5
ADC1_SE18
  //( 1, PTZ     ,  8,           , PTZ8    ), // PTZ8
// };

const pin_obj_t pin_Z8 = PIN(Z, 8, 0, NULL, 0, 0);

// const pin_af_obj_t pin_Z9_af[] = {
  //( 0, DAC     ,  0, OUT
CMP1_IN3
ADC0_SE23, DAC0    ), // DAC0_OUT
CMP1_IN3
ADC0_SE23
  //( 1, PTZ     ,  9,           , PTZ9    ), // PTZ9
// };

const pin_obj_t pin_Z9 = PIN(Z, 9, 0, NULL, 0, 0);

const pin_af_obj_t pin_E24_af[] = {
  //( 0, ADC     ,  0, SE17      , ADC0    ), // ADC0_SE17
  //( 1, PTE     , 24,           , PTE24   ), // PTE24
  AF( 3, UART    ,  4, TX        , UART4   ), // UART4_TX
  AF( 5, I2C     ,  0, SCL       , I2C0    ), // I2C0_SCL
  //( 6, EWM     ,  0, OUT_b     , EWM     ), // EWM_OUT_b
};

const pin_obj_t pin_E24 = PIN(E, 24, 2, pin_E24_af, 0, 0);

const pin_af_obj_t pin_E25_af[] = {
  //( 0, ADC     ,  0, SE18      , ADC0    ), // ADC0_SE18
  //( 1, PTE     , 25,           , PTE25   ), // PTE25
  AF( 3, UART    ,  4, RX        , UART4   ), // UART4_RX
  AF( 5, I2C     ,  0, SDA       , I2C0    ), // I2C0_SDA
  //( 6, EWM     ,  0, IN        , EWM     ), // EWM_IN
};

const pin_obj_t pin_E25 = PIN(E, 25, 2, pin_E25_af, 0, 0);

// const pin_af_obj_t pin_E26_af[] = {
  //( 1, PTE     , 26,           , PTE26   ), // PTE26
  //( 2, ENET    ,  0, 1588_CLKIN, ENET    ), // ENET_1588_CLKIN
  //( 3, UART    ,  4, CTS_b     , UART4   ), // UART4_CTS_b
  //( 6, RTC     ,  0, CLKOUT    , RTC     ), // RTC_CLKOUT
  //( 7, USB     ,  0, CLKIN     , USB     ), // USB_CLKIN
// };

const pin_obj_t pin_E26 = PIN(E, 26, 0, NULL, 0, 0);

const pin_af_obj_t pin_A0_af[] = {
  //( 1, PTA     ,  0,           , PTA0    ), // PTA0
  //( 2, UART    ,  0, CTS_b
UART0_COL_b, UART0   ), // UART0_CTS_b
UART0_COL_b
  AF( 3, FTM     ,  0, CH5       , FTM0    ), // FTM0_CH5
  //( 7, JTAG    ,  0, TCLK
SWD_CLK, JTAG    ), // JTAG_TCLK
SWD_CLK
  //( 8, EZP     ,  0, CLK       , EZP     ), // EZP_CLK
};

const pin_obj_t pin_A0 = PIN(A, 0, 1, pin_A0_af, 0, 0);

const pin_af_obj_t pin_A1_af[] = {
  //( 1, PTA     ,  1,           , PTA1    ), // PTA1
  AF( 2, UART    ,  0, RX        , UART0   ), // UART0_RX
  AF( 3, FTM     ,  0, CH6       , FTM0    ), // FTM0_CH6
  //( 7, JTAG    ,  0, TDI       , JTAG    ), // JTAG_TDI
  //( 8, EZP     ,  0, DI        , EZP     ), // EZP_DI
};

const pin_obj_t pin_A1 = PIN(A, 1, 2, pin_A1_af, 0, 0);

const pin_af_obj_t pin_A2_af[] = {
  //( 1, PTA     ,  2,           , PTA2    ), // PTA2
  AF( 2, UART    ,  0, TX        , UART0   ), // UART0_TX
  AF( 3, FTM     ,  0, CH7       , FTM0    ), // FTM0_CH7
  //( 7, JTAG    ,  0, TDO
TRACE_SWO, JTAG    ), // JTAG_TDO
TRACE_SWO
  //( 8, EZP     ,  0, DO        , EZP     ), // EZP_DO
};

const pin_obj_t pin_A2 = PIN(A, 2, 2, pin_A2_af, 0, 0);

const pin_af_obj_t pin_B2_af[] = {
  //( 0, ADC     ,  0, SE12      , ADC0    ), // ADC0_SE12
  //( 1, PTB     ,  2,           , PTB2    ), // PTB2
  AF( 2, I2C     ,  0, SCL       , I2C0    ), // I2C0_SCL
  //( 3, UART    ,  0, RTS_b     , UART0   ), // UART0_RTS_b
  //( 4, ENET    ,  0, 1588_TMR0 , ENET0   ), // ENET0_1588_TMR0
  //( 6, FTM     ,  0, FLT3      , FTM0    ), // FTM0_FLT3
};

const pin_obj_t pin_B2 = PIN(B, 2, 1, pin_B2_af, 0, 0);

const pin_af_obj_t pin_B3_af[] = {
  //( 0, ADC     ,  0, SE13      , ADC0    ), // ADC0_SE13
  //( 1, PTB     ,  3,           , PTB3    ), // PTB3
  AF( 2, I2C     ,  0, SDA       , I2C0    ), // I2C0_SDA
  //( 3, UART    ,  0, CTS_b
UART0_COL_b, UART0   ), // UART0_CTS_b
UART0_COL_b
  //( 4, ENET    ,  0, 1588_TMR1 , ENET0   ), // ENET0_1588_TMR1
  //( 6, FTM     ,  0, FLT0      , FTM0    ), // FTM0_FLT0
};

const pin_obj_t pin_B3 = PIN(B, 3, 1, pin_B3_af, 0, 0);

// const pin_af_obj_t pin_B9_af[] = {
  //( 1, PTB     ,  9,           , PTB9    ), // PTB9
  //( 2, SPI     ,  1, PCS1      , SPI1    ), // SPI1_PCS1
  //( 3, UART    ,  3, CTS_b     , UART3   ), // UART3_CTS_b
  //( 5, FB      ,  0, AD20      , FB      ), // FB_AD20
// };

const pin_obj_t pin_B9 = PIN(B, 9, 0, NULL, 0, 0);

const pin_af_obj_t pin_B10_af[] = {
  //( 0, ADC     ,  1, SE14      , ADC1    ), // ADC1_SE14
  //( 1, PTB     , 10,           , PTB10   ), // PTB10
  //( 2, SPI     ,  1, PCS0      , SPI1    ), // SPI1_PCS0
  AF( 3, UART    ,  3, RX        , UART3   ), // UART3_RX
  //( 5, FB      ,  0, AD19      , FB      ), // FB_AD19
  //( 6, FTM     ,  0, FLT1      , FTM0    ), // FTM0_FLT1
};

const pin_obj_t pin_B10 = PIN(B, 10, 1, pin_B10_af, 0, 0);

const pin_af_obj_t pin_B11_af[] = {
  //( 0, ADC     ,  1, SE15      , ADC1    ), // ADC1_SE15
  //( 1, PTB     , 11,           , PTB11   ), // PTB11
  AF( 2, SPI     ,  1, SCK       , SPI1    ), // SPI1_SCK
  AF( 3, UART    ,  3, TX        , UART3   ), // UART3_TX
  //( 5, FB      ,  0, AD18      , FB      ), // FB_AD18
  //( 6, FTM     ,  0, FLT2      , FTM0    ), // FTM0_FLT2
};

const pin_obj_t pin_B11 = PIN(B, 11, 2, pin_B11_af, 0, 0);

const pin_af_obj_t pin_B18_af[] = {
  //( 1, PTB     , 18,           , PTB18   ), // PTB18
  //( 2, CAN     ,  0, TX        , CAN0    ), // CAN0_TX
  AF( 3, FTM     ,  2, CH0       , FTM2    ), // FTM2_CH0
  //( 4, I2S     ,  0, TX_BCLK   , I2S0    ), // I2S0_TX_BCLK
  //( 5, FB      ,  0, AD15      , FB      ), // FB_AD15
  AF( 6, FTM     ,  2, QD_PHA    , FTM2    ), // FTM2_QD_PHA
};

const pin_obj_t pin_B18 = PIN(B, 18, 2, pin_B18_af, 0, 0);

const pin_af_obj_t pin_B19_af[] = {
  //( 1, PTB     , 19,           , PTB19   ), // PTB19
  //( 2, CAN     ,  0, RX        , CAN0    ), // CAN0_RX
  AF( 3, FTM     ,  2, CH1       , FTM2    ), // FTM2_CH1
  //( 4, I2S     ,  0, TX_FS     , I2S0    ), // I2S0_TX_FS
  //( 5, FB      ,  0, OE_b      , FB      ), // FB_OE_b
  AF( 6, FTM     ,  2, QD_PHB    , FTM2    ), // FTM2_QD_PHB
};

const pin_obj_t pin_B19 = PIN(B, 19, 2, pin_B19_af, 0, 0);

// const pin_af_obj_t pin_B20_af[] = {
  //( 1, PTB     , 20,           , PTB20   ), // PTB20
  //( 2, SPI     ,  2, PCS0      , SPI2    ), // SPI2_PCS0
  //( 5, FB      ,  0, AD31      , FB      ), // FB_AD31
  //( 6, CMP     ,  0, OUT       , CMP0    ), // CMP0_OUT
// };

const pin_obj_t pin_B20 = PIN(B, 20, 0, NULL, 0, 0);

const pin_af_obj_t pin_B21_af[] = {
  //( 1, PTB     , 21,           , PTB21   ), // PTB21
  AF( 2, SPI     ,  2, SCK       , SPI2    ), // SPI2_SCK
  //( 5, FB      ,  0, AD30      , FB      ), // FB_AD30
  //( 6, CMP     ,  1, OUT       , CMP1    ), // CMP1_OUT
};

const pin_obj_t pin_B21 = PIN(B, 21, 1, pin_B21_af, 0, 0);

// const pin_af_obj_t pin_B22_af[] = {
  //( 1, PTB     , 22,           , PTB22   ), // PTB22
  //( 2, SPI     ,  2, SOUT      , SPI2    ), // SPI2_SOUT
  //( 5, FB      ,  0, AD29      , FB      ), // FB_AD29
  //( 6, CMP     ,  2, OUT       , CMP2    ), // CMP2_OUT
// };

const pin_obj_t pin_B22 = PIN(B, 22, 0, NULL, 0, 0);

// const pin_af_obj_t pin_B23_af[] = {
  //( 1, PTB     , 23,           , PTB23   ), // PTB23
  //( 2, SPI     ,  2, SIN       , SPI2    ), // SPI2_SIN
  //( 3, SPI     ,  0, PCS5      , SPI0    ), // SPI0_PCS5
  //( 5, FB      ,  0, AD28      , FB      ), // FB_AD28
// };

const pin_obj_t pin_B23 = PIN(B, 23, 0, NULL, 0, 0);

// const pin_af_obj_t pin_C0_af[] = {
  //( 0, ADC     ,  0, SE14      , ADC0    ), // ADC0_SE14
  //( 1, PTC     ,  0,           , PTC0    ), // PTC0
  //( 2, SPI     ,  0, PCS4      , SPI0    ), // SPI0_PCS4
  //( 3, PDB     ,  0, EXTRG     , PDB0    ), // PDB0_EXTRG
  //( 4, USB     ,  0, SOF_OUT   , USB     ), // USB_SOF_OUT
  //( 5, FB      ,  0, AD14      , FB      ), // FB_AD14
  //( 6, I2S     ,  0, TXD1      , I2S0    ), // I2S0_TXD1
// };

const pin_obj_t pin_C0 = PIN(C, 0, 0, NULL, 0, 0);

const pin_af_obj_t pin_C2_af[] = {
  //( 0, ADC     ,  0, SE4b
CMP1_IN0, ADC0    ), // ADC0_SE4b
CMP1_IN0
  //( 1, PTC     ,  2,           , PTC2    ), // PTC2
  //( 2, SPI     ,  0, PCS2      , SPI0    ), // SPI0_PCS2
  //( 3, UART    ,  1, CTS_b     , UART1   ), // UART1_CTS_b
  AF( 4, FTM     ,  0, CH1       , FTM0    ), // FTM0_CH1
  //( 5, FB      ,  0, AD12      , FB      ), // FB_AD12
  //( 6, I2S     ,  0, TX_FS     , I2S0    ), // I2S0_TX_FS
};

const pin_obj_t pin_C2 = PIN(C, 2, 1, pin_C2_af, 0, 0);

// const pin_af_obj_t pin_C7_af[] = {
  //( 0, CMP     ,  0, IN1       , CMP0    ), // CMP0_IN1
  //( 1, PTC     ,  7,           , PTC7    ), // PTC7
  //( 2, SPI     ,  0, SIN       , SPI0    ), // SPI0_SIN
  //( 3, USB     ,  0, SOF_OUT   , USB     ), // USB_SOF_OUT
  //( 4, I2S     ,  0, RX_FS     , I2S0    ), // I2S0_RX_FS
  //( 5, FB      ,  0, AD8       , FB      ), // FB_AD8
// };

const pin_obj_t pin_C7 = PIN(C, 7, 0, NULL, 0, 0);

const pin_af_obj_t pin_C8_af[] = {
  //( 0, ADC     ,  1, SE4b
CMP0_IN2, ADC1    ), // ADC1_SE4b
CMP0_IN2
  //( 1, PTC     ,  8,           , PTC8    ), // PTC8
  AF( 3, FTM     ,  3, CH4       , FTM3    ), // FTM3_CH4
  //( 4, I2S     ,  0, MCLK      , I2S0    ), // I2S0_MCLK
  //( 5, FB      ,  0, AD7       , FB      ), // FB_AD7
};

const pin_obj_t pin_C8 = PIN(C, 8, 1, pin_C8_af, 0, 0);

const pin_af_obj_t pin_C9_af[] = {
  //( 0, ADC     ,  1, SE5b
CMP0_IN3, ADC1    ), // ADC1_SE5b
CMP0_IN3
  //( 1, PTC     ,  9,           , PTC9    ), // PTC9
  AF( 3, FTM     ,  3, CH5       , FTM3    ), // FTM3_CH5
  //( 4, I2S     ,  0, RX_BCLK   , I2S0    ), // I2S0_RX_BCLK
  //( 5, FB      ,  0, AD6       , FB      ), // FB_AD6
  //( 6, FTM     ,  2, FLT0      , FTM2    ), // FTM2_FLT0
};

const pin_obj_t pin_C9 = PIN(C, 9, 1, pin_C9_af, 0, 0);

const pin_af_obj_t pin_C10_af[] = {
  //( 0, ADC     ,  1, SE6b      , ADC1    ), // ADC1_SE6b
  //( 1, PTC     , 10,           , PTC10   ), // PTC10
  AF( 2, I2C     ,  1, SCL       , I2C1    ), // I2C1_SCL
  AF( 3, FTM     ,  3, CH6       , FTM3    ), // FTM3_CH6
  //( 4, I2S     ,  0, RX_FS     , I2S0    ), // I2S0_RX_FS
  //( 5, FB      ,  0, AD5       , FB      ), // FB_AD5
};

const pin_obj_t pin_C10 = PIN(C, 10, 2, pin_C10_af, 0, 0);

const pin_af_obj_t pin_C16_af[] = {
  //( 1, PTC     , 16,           , PTC16   ), // PTC16
  AF( 3, UART    ,  3, RX        , UART3   ), // UART3_RX
  //( 4, ENET    ,  0, 1588_TMR0 , ENET0   ), // ENET0_1588_TMR0
  //( 5, FB      ,  0, CS5_b
FB_TSIZ1
FB_BE23_16_BLS15_8_b, FB      ), // FB_CS5_b
FB_TSIZ1
FB_BE23_16_BLS15_8_b
};

const pin_obj_t pin_C16 = PIN(C, 16, 1, pin_C16_af, 0, 0);

const pin_af_obj_t pin_C17_af[] = {
  //( 1, PTC     , 17,           , PTC17   ), // PTC17
  AF( 3, UART    ,  3, TX        , UART3   ), // UART3_TX
  //( 4, ENET    ,  0, 1588_TMR1 , ENET0   ), // ENET0_1588_TMR1
  //( 5, FB      ,  0, CS4_b
FB_TSIZ0
FB_BE31_24_BLS7_0_b, FB      ), // FB_CS4_b
FB_TSIZ0
FB_BE31_24_BLS7_0_b
};

const pin_obj_t pin_C17 = PIN(C, 17, 1, pin_C17_af, 0, 0);

const pin_af_obj_t pin_D1_af[] = {
  //( 0, ADC     ,  0, SE5b      , ADC0    ), // ADC0_SE5b
  //( 1, PTD     ,  1,           , PTD1    ), // PTD1
  AF( 2, SPI     ,  0, SCK       , SPI0    ), // SPI0_SCK
  //( 3, UART    ,  2, CTS_b     , UART2   ), // UART2_CTS_b
  AF( 4, FTM     ,  3, CH1       , FTM3    ), // FTM3_CH1
  //( 5, FB      ,  0, CS0_b     , FB      ), // FB_CS0_b
};

const pin_obj_t pin_D1 = PIN(D, 1, 2, pin_D1_af, 0, 0);

const pin_af_obj_t pin_D3_af[] = {
  //( 1, PTD     ,  3,           , PTD3    ), // PTD3
  //( 2, SPI     ,  0, SIN       , SPI0    ), // SPI0_SIN
  AF( 3, UART    ,  2, TX        , UART2   ), // UART2_TX
  AF( 4, FTM     ,  3, CH3       , FTM3    ), // FTM3_CH3
  //( 5, FB      ,  0, AD3       , FB      ), // FB_AD3
  AF( 7, I2C     ,  0, SDA       , I2C0    ), // I2C0_SDA
};

const pin_obj_t pin_D3 = PIN(D, 3, 3, pin_D3_af, 0, 0);

STATIC const mp_map_elem_t pin_cpu_pins_locals_dict_table[] = {
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z0), (mp_obj_t)&pin_Z0 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z1), (mp_obj_t)&pin_Z1 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z2), (mp_obj_t)&pin_Z2 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z3), (mp_obj_t)&pin_Z3 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z4), (mp_obj_t)&pin_Z4 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z5), (mp_obj_t)&pin_Z5 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z6), (mp_obj_t)&pin_Z6 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z7), (mp_obj_t)&pin_Z7 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z8), (mp_obj_t)&pin_Z8 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_Z9), (mp_obj_t)&pin_Z9 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_E24), (mp_obj_t)&pin_E24 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_E25), (mp_obj_t)&pin_E25 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_E26), (mp_obj_t)&pin_E26 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A0), (mp_obj_t)&pin_A0 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A1), (mp_obj_t)&pin_A1 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A2), (mp_obj_t)&pin_A2 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B2), (mp_obj_t)&pin_B2 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B3), (mp_obj_t)&pin_B3 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B9), (mp_obj_t)&pin_B9 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B10), (mp_obj_t)&pin_B10 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B11), (mp_obj_t)&pin_B11 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B18), (mp_obj_t)&pin_B18 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B19), (mp_obj_t)&pin_B19 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B20), (mp_obj_t)&pin_B20 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B21), (mp_obj_t)&pin_B21 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B22), (mp_obj_t)&pin_B22 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_B23), (mp_obj_t)&pin_B23 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_C0), (mp_obj_t)&pin_C0 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_C2), (mp_obj_t)&pin_C2 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_C7), (mp_obj_t)&pin_C7 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_C8), (mp_obj_t)&pin_C8 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_C9), (mp_obj_t)&pin_C9 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_C10), (mp_obj_t)&pin_C10 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_C16), (mp_obj_t)&pin_C16 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_C17), (mp_obj_t)&pin_C17 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D1), (mp_obj_t)&pin_D1 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D3), (mp_obj_t)&pin_D3 },
};
MP_DEFINE_CONST_DICT(pin_cpu_pins_locals_dict, pin_cpu_pins_locals_dict_table);

STATIC const mp_map_elem_t pin_board_pins_locals_dict_table[] = {
  { MP_OBJ_NEW_QSTR(MP_QSTR_A00), (mp_obj_t)&pin_Z0 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A01), (mp_obj_t)&pin_Z1 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A02), (mp_obj_t)&pin_Z2 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A03), (mp_obj_t)&pin_Z3 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A04), (mp_obj_t)&pin_Z4 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A05), (mp_obj_t)&pin_Z5 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A06), (mp_obj_t)&pin_Z6 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A07), (mp_obj_t)&pin_Z7 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A08), (mp_obj_t)&pin_Z8 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_A09), (mp_obj_t)&pin_Z9 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D00), (mp_obj_t)&pin_B18 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D01), (mp_obj_t)&pin_C16 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D02), (mp_obj_t)&pin_B19 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D03), (mp_obj_t)&pin_C17 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D05), (mp_obj_t)&pin_B9 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D06), (mp_obj_t)&pin_C8 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D07), (mp_obj_t)&pin_A1 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D08), (mp_obj_t)&pin_C9 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D09), (mp_obj_t)&pin_B23 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D10), (mp_obj_t)&pin_C0 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D11), (mp_obj_t)&pin_A2 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D12), (mp_obj_t)&pin_C7 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D13), (mp_obj_t)&pin_C2 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D16), (mp_obj_t)&pin_E26 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D17), (mp_obj_t)&pin_A0 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D21), (mp_obj_t)&pin_D3 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D22), (mp_obj_t)&pin_D1 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D23), (mp_obj_t)&pin_E25 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D24), (mp_obj_t)&pin_E24 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D25), (mp_obj_t)&pin_B2 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D26), (mp_obj_t)&pin_B3 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D27), (mp_obj_t)&pin_B10 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D28), (mp_obj_t)&pin_B11 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D30), (mp_obj_t)&pin_C10 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_D31), (mp_obj_t)&pin_B20 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_LedGrn), (mp_obj_t)&pin_B21 },
  { MP_OBJ_NEW_QSTR(MP_QSTR_LedRed), (mp_obj_t)&pin_B22 },
};
MP_DEFINE_CONST_DICT(pin_board_pins_locals_dict, pin_board_pins_locals_dict_table);

const pin_obj_t * const pin_adc1[] = {
  NULL,    // 0
  NULL,    // 1
  NULL,    // 2
  NULL,    // 3
  NULL,    // 4
  NULL,    // 5
  NULL,    // 6
  NULL,    // 7
  NULL,    // 8
  NULL,    // 9
  NULL,    // 10
  NULL,    // 11
  NULL,    // 12
  NULL,    // 13
  NULL,    // 14
  NULL,    // 15
};

const pin_obj_t * const pin_adc2[] = {
  NULL,    // 0
  NULL,    // 1
  NULL,    // 2
  NULL,    // 3
  NULL,    // 4
  NULL,    // 5
  NULL,    // 6
  NULL,    // 7
  NULL,    // 8
  NULL,    // 9
  NULL,    // 10
  NULL,    // 11
  NULL,    // 12
  NULL,    // 13
  NULL,    // 14
  NULL,    // 15
};

const pin_obj_t * const pin_adc3[] = {
  NULL,    // 0
  NULL,    // 1
  NULL,    // 2
  NULL,    // 3
  NULL,    // 4
  NULL,    // 5
  NULL,    // 6
  NULL,    // 7
  NULL,    // 8
  NULL,    // 9
  NULL,    // 10
  NULL,    // 11
  NULL,    // 12
  NULL,    // 13
  NULL,    // 14
  NULL,    // 15
};