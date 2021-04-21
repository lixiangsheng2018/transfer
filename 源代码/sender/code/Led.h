#ifndef _LED_H_
#define _LED_H_
#include "stdbool.h"
#include "nrf.h"
#include "defines.h"

#define  LED_NUM 3
#define  LED_BATIND_DURATION  40
#define  LED_FLASH_NUM        5

#define LED_TOGGLE(p) do {if((NRF_P0->OUT) & ((uint32_t)1 << (p))) NRF_P0->OUTCLR = ((uint32_t)1 << (p)); else NRF_P0->OUTSET = ((uint32_t)1 << (p));} while(0);
#define LED_ON(p)     do {NRF_P0->OUTSET = ((uint32_t)1 << p);} while(0);
#define LED_OFF(p)    do {NRF_P0->OUTCLR = ((uint32_t)1 << p);} while(0);

typedef enum _CHARGE_STATE {
    CHARGE_STATE_OFF = 0,
    CHARGE_STATE_ON,
    CHARGE_STATE_FULL
}ChargeState;

typedef enum _LED_STATE {
    LED_STATE_OFF = 0,
    LED_STATE_ON,
    LED_STATE_FLASH,
}LedState;

typedef enum _LED_ALIAS {
    STATE_LED = 0,
    CHARGING_LED,
    STANDBY_LED,
    BATLEVEL_IND_LED,
}LedId;

typedef struct _LED_PIN {
	  uint32_t  pin;
	  LedState  state;
}LedPin;

void Led_Init( void );

void SetLedState( LedId led, LedState state);
bool LedIsBusy( void );
#endif

