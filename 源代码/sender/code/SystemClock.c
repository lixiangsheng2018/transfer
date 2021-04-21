#include "Sender.h"
#include "nrf_delay.h"

__IO uint32_t SystemTicks;

#define ONE_SECOND   32768
#define RELOAD       0x6E524635
#define SLEEP_BIT    0
#define HALT_BIT     3
#define WDT_RELOAD   NRF_WDT->RR[0]=RELOAD

void Watchdog_Init(uint32_t crvSeconds)
{	
    NRF_WDT->CRV = crvSeconds*ONE_SECOND;	
    NRF_WDT->CONFIG |= 1<<SLEEP_BIT | 1<<HALT_BIT;	
    NRF_WDT->RREN |= 1<<0;	
    NRF_WDT->RR[0]=RELOAD;	
    NRF_WDT->TASKS_START = 1U;
}

void Watchdog_Feed(void)
{
    WDT_RELOAD;
}

void SystemClock_Init(void){

    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while(0 == NRF_CLOCK->EVENTS_HFCLKSTARTED);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->LFCLKSRC = (0 << 0) | (0 << 16) | (0 << 17);
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while(0 == NRF_CLOCK->EVENTS_LFCLKSTARTED);

    return ;
}

unsigned int RND_Get(void){
    unsigned int val;
    NRF_RNG->CONFIG = 1;
    NRF_RNG->EVENTS_VALRDY = 0;
    NRF_RNG->TASKS_START = 1;
    while (0 == NRF_RNG->EVENTS_VALRDY);
    val = NRF_RNG->VALUE;
    NRF_RNG->EVENTS_VALRDY = 0;
    NRF_RNG->TASKS_STOP = 1;
    
    return val;
}

void RTC0_IRQHandler(void){

    if (NRF_RTC0->EVENTS_COMPARE[0]) {
        NRF_RTC0->EVENTS_COMPARE[0] = 0;
        (void)(NRF_RTC0->EVENTS_COMPARE[0]);
        NRF_RTC0->TASKS_CLEAR = 1;
        SystemTicks++;
    }
}

void SystemTick_Init(void){

    SystemTicks = 0;
    NRF_RTC0->TASKS_STOP = 1;
    NRF_RTC0->TASKS_CLEAR = 1;
    NRF_RTC0->PRESCALER = 0;
    NRF_RTC0->CC[0] = 32767;
    NRF_RTC0->INTENSET = (1 << 16);//enable CC0
    NRF_RTC0->TASKS_START = 1;
    NVIC_EnableIRQ(RTC0_IRQn);
}

uint32_t SystemTick_Get(void){

    uint32_t st;
    uint32_t count;

    /* NOTE: TASKS_CLEAR will effect AFTER this input clock is finish */
    NVIC_DisableIRQ(RTC0_IRQn);
    st = SystemTicks;
    count = (NRF_RTC0->COUNTER);
    while (count == (NRF_RTC0->COUNTER));
    count = (NRF_RTC0->COUNTER);
    NVIC_EnableIRQ(RTC0_IRQn);

    count = (uint32_t)((count * 1000) / 32768);
    return st * 1000 + count;
}

uint32_t SystemTick_GetTicks(void){

    uint32_t st;
    uint32_t count;

    /* NOTE: TASKS_CLEAR will effect AFTER this input clock is finish */
    NVIC_DisableIRQ(RTC0_IRQn);
    st = SystemTicks;
    count = (NRF_RTC0->COUNTER);
    while (count == (NRF_RTC0->COUNTER));
    count = (NRF_RTC0->COUNTER);
    NVIC_EnableIRQ(RTC0_IRQn);

    return st * 32768 + count;
}

uint32_t SystemTick_GetSecond(void){
    return SystemTicks;
}

void Delay_ms( uint32_t ms )
{
    uint32_t count = ms/50;
    while( count-- ){
        nrf_delay_ms(50);
        Watchdog_Feed();
#ifdef BATTERY_LEVEL_DETECT
        BatteryLevelDetect();	
#endif		
    }
}
