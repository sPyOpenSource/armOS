
#ifndef MODPINMAP_H
#define MODPINMAP_H
/*

Use the Aruino pin mapping structure

*/

#include "py/headers/obj.h"
#include "due_mphal.h"

#define PIN_ATTR_COMBO         (1UL<<0)
#define PIN_ATTR_ANALOG        (1UL<<1)
#define PIN_ATTR_DIGITAL       (1UL<<2)
#define PIN_ATTR_PWM           (1UL<<3)
#define PIN_ATTR_TIMER         (1UL<<4)

#define PINS_COUNT           (79u)

#define PIN_STATUS_DIGITAL_INPUT_PULLUP  (0x01)
#define PIN_STATUS_DIGITAL_INPUT         (0x02)
#define PIN_STATUS_DIGITAL_OUTPUT        (0x03)
#define PIN_STATUS_ANALOG                (0x04)
#define PIN_STATUS_PWM                   (0x05)
#define PIN_STATUS_TIMER                 (0x06)
#define PIN_STATUS_SERIAL                (0x07)
#define PIN_STATUS_DW_LOW                (0x10)
#define PIN_STATUS_DW_HIGH               (0x11)


#define INPUT                 0x0
#define OUTPUT                0x1
#define INPUT_PULLUP          0x2

#define DACC_INTERFACE        DACC
#define DACC_INTERFACE_ID     ID_DACC
#define DACC_RESOLUTION       12
#define DACC_ISR_HANDLER      DACC_Handler
#define DACC_ISR_ID           DACC_IRQn


#define PWM_INTERFACE   PWM
#define PWM_INTERFACE_ID  ID_PWM
#define PWM_RESOLUTION    8


#define NOT_A_PORT          0

#define NOT_AN_INTERRUPT   -1

#define A0    54
#define A1    55
#define A2    56
#define A3    57
#define A4    58
#define A5    59
#define A6    60
#define A7    61
#define A8    62
#define A9    63
#define A10   64
#define A11   65
#define DAC0  66
#define DAC1  67
#define CANRX 68
#define CANTX 69
#define ADC_RESOLUTION 12


#define I2C_BUS_1 1 // ( Pins 20,21)
#define I2C_BUS_0 2//  (Pins 70,71)


#define I2C_SDA0  70
#define I2C_SCL0  71
// Id for Pins 70,71 is ID_TWI1

#define I2C_SDA1  20  
#define I2C_SCL1  21
// Id for Pins 20,21 is ID_TWI0


typedef enum _EExt_Interrupts
{
  EXTERNAL_INT_0 = 0,
  EXTERNAL_INT_1 = 1,
  EXTERNAL_INT_2 = 2,
  EXTERNAL_INT_3 = 3,
  EXTERNAL_INT_4 = 4,
  EXTERNAL_INT_5 = 5,
  EXTERNAL_INT_6 = 6,
  EXTERNAL_INT_7 = 7,
  EXTERNAL_NUM_INTERRUPTS
} EExt_Interrupts ;

typedef enum _EAnalogChannel
{
  NO_ADC = -1,
  ADC0   = 0,
  ADC1,
  ADC2,
  ADC3,
  ADC4,
  ADC5,
  ADC6,
  ADC7,
  ADC8,
  ADC9,
  ADC10,
  ADC11,
  ADC12,
  ADC13,
  ADC14,
  ADC15,
  DA0,
  DA1
} EAnalogChannel ;

#define ADC_CHANNEL_NUMBER_NONE 0xffffffff

// Definitions for PWM channels
typedef enum _EPWMChannel
{
  NOT_ON_PWM = -1,
  PWM_CH0    = 0,
  PWM_CH1,
  PWM_CH2,
  PWM_CH3,
  PWM_CH4,
  PWM_CH5,
  PWM_CH6,
  PWM_CH7
} EPWMChannel ;

// Definitions for TC channels
typedef enum _ETCChannel
{
  NOT_ON_TIMER = -1,
  TC0_CHA0     = 0,
  TC0_CHB0,
  TC0_CHA1,
  TC0_CHB1,
  TC0_CHA2,
  TC0_CHB2,
  TC1_CHA3,
  TC1_CHB3,
  TC1_CHA4,
  TC1_CHB4,
  TC1_CHA5,
  TC1_CHB5,
  TC2_CHA6,
  TC2_CHB6,
  TC2_CHA7,
  TC2_CHB7,
  TC2_CHA8,
  TC2_CHB8
} ETCChannel ;


typedef struct _PinDescription
{
  mp_obj_base_t base;
  uint8_t board_pin;
  Pio* pPort ;
  uint32_t cpu_pin;
  uint32_t ulPeripheralId ;
  pio_type_t ulPinType ;
  uint32_t ulPinConfiguration ;
  uint32_t ulPinAttribute ;
  EAnalogChannel ulAnalogChannel ; /* Analog pin in the Arduino context (label on the board) */
  EAnalogChannel ulADCChannelNumber ; /* ADC Channel number in the SAM device */
  EPWMChannel ulPWMChannel ;
  ETCChannel ulTCChannel ;
} PinDescription ;

extern uint8_t g_pinStatus[];
extern char g_pinMode[];

extern const PinDescription g_APinDescription[];

typedef PinDescription pyb_pin_obj;

const mp_obj_type_t pin_type;
extern const mp_obj_type_t pin_board_pins_obj_type;
extern const mp_obj_dict_t pin_board_pins_locals_dict;

void pin_init0(void);
const pyb_pin_obj *pin_find(mp_obj_t user_obj);
const pyb_pin_obj *pin_find_named_pin(const mp_obj_dict_t *named_pins, mp_obj_t name);


#endif // MODPINMAP_H