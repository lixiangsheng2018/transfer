#include "Receiver.h"

static volatile unsigned int SystemTicks;
static volatile unsigned int ForceSleep;

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
    NRF_RTC0->INTENSET = (1 << 16);//Enable CC0
    NRF_RTC0->TASKS_START = 1;
    NVIC_EnableIRQ(RTC0_IRQn);
}

unsigned int SystemTick_Get(void){

    unsigned int st;
    unsigned int count;

    /* NOTE: TASKS_CLEAR will effect AFTER this input clock is finish */
    NVIC_DisableIRQ(RTC0_IRQn);
    st = SystemTicks;
    count = (NRF_RTC0->COUNTER);
    while (count == (NRF_RTC0->COUNTER));
    count = (NRF_RTC0->COUNTER);
    NVIC_EnableIRQ(RTC0_IRQn);

    count = (unsigned int)((count * 1000) / 32768);
    return st * 1000 + count;
}

unsigned int SystemTick_GetTicks(void){

    unsigned int st;
    unsigned int count;

    /* NOTE: TASKS_CLEAR will effect AFTER this input clock is finish */
    NVIC_DisableIRQ(RTC0_IRQn);
    st = SystemTicks;
    count = (NRF_RTC0->COUNTER);
    while (count == (NRF_RTC0->COUNTER));
    count = (NRF_RTC0->COUNTER);
    NVIC_EnableIRQ(RTC0_IRQn);

    return st * 32768 + count;
}


void RTC1_IRQHandler(void){

    if (NRF_RTC1->EVENTS_COMPARE[0]) {
        NRF_RTC1->EVENTS_COMPARE[0] = 0;
        (void)(NRF_RTC1->EVENTS_COMPARE[0]);
        NRF_RTC1->TASKS_CLEAR = 1;
        ForceSleep = 0;
    }
}

void SystemTick_ForceSleepMs(unsigned int ms){
#if ( (RECV_MODE & RECV_MODE_POWER) == 0 )
    NRF_RTC1->TASKS_STOP = 1;
    NRF_RTC1->TASKS_CLEAR = 1;
    NRF_RTC1->PRESCALER = 0;
    NRF_RTC1->CC[0] = (unsigned int)(32.767 * ms + 0.5);
    NRF_RTC1->INTENSET = (1 << 16);//Enable CC0
    NRF_RTC1->TASKS_START = 1;
    NVIC_EnableIRQ(RTC1_IRQn);

    ForceSleep = 1;
    while (ForceSleep) {
        __WFE();
    }
#endif    
}

void SystemTick_ForceSleepTicks(unsigned int ticks){
#if ( (RECV_MODE & RECV_MODE_POWER) == 0 )
    NRF_RTC1->TASKS_STOP = 1;
    NRF_RTC1->TASKS_CLEAR = 1;
    NRF_RTC1->PRESCALER = 0;
    NRF_RTC1->CC[0] = ticks;
    NRF_RTC1->INTENSET = (1 << 16);//Enable CC0
    NRF_RTC1->TASKS_START = 1;
    NVIC_EnableIRQ(RTC1_IRQn);

    ForceSleep = 1;
    while (ForceSleep) {
        __WFE();
    }
#endif    
}

bool User_WFI(unsigned int ms)
{
    NRF_RTC1->TASKS_STOP = 1;
    NRF_RTC1->TASKS_CLEAR = 1;
    NRF_RTC1->PRESCALER = 0;
    NRF_RTC1->CC[0] = (unsigned int)(32.767 * ms + 0.5);
    NRF_RTC1->INTENSET = (1 << 16);//Enable CC0
    NRF_RTC1->TASKS_START = 1;
    NVIC_EnableIRQ(RTC1_IRQn);

    ForceSleep = 1;
    while (ForceSleep  && ReceiverSC.FlagRfRecv == 0) {
        __WFI();
    }
    
    return  ReceiverSC.FlagRfRecv;
}
