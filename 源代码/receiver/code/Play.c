#include "Receiver.h"
#include "nrf_delay.h"

static unsigned short PWM_Duty[2] = {0x8080,0x8080};
static signed short LastPlayPoint = 0x8000;
static signed short PlayState = 0;

#define PLAY_BITS    8
#define PLAY_MASK    ((1<<PLAY_BITS)-1)


void TIMER1_IRQHandler(void){

    signed short * p = (signed short *)(ReceiverSC.PlayBufPointer);
    unsigned int PlayVal;

    if( (ReceiverSC.TipsPlayPointer && ReceiverSC.PlayBufPlayPos < ReceiverSC.PlayBufRecvPos) ||
        (ReceiverSC.PlayBufPlayPos + PLAY_AUDIO_DELAY_FRAMES*IMA_ADPCM_PCM_RAW_LEN < ReceiverSC.PlayBufRecvPos)){
        p += (ReceiverSC.PlayBufPlayPos % ReceiverSC.PlayBufLen);
        ReceiverSC.PlayBufPlayPos++;
        LastPlayPoint = (*p);
    }else{
        LastPlayPoint = 0x0000;
    }
    
    int Enhance = LastPlayPoint * ReceiverSC.PlayVol;
    if (Enhance > 32767) {
        Enhance = 32767;
    }else if(Enhance < -32768){
        Enhance = -32768;
    }
    LastPlayPoint = (signed short)(Enhance);

    //Denoise.
    if((LastPlayPoint < QUIET_THREHOLD) && (LastPlayPoint > -QUIET_THREHOLD)) LastPlayPoint = 0;
    
    PlayVal = LastPlayPoint + 32768;
    PlayVal >>= (16-PLAY_BITS);

    PWM_Duty[0] = 0x8000 + ((PlayVal) & PLAY_MASK); 
    PWM_Duty[1] = 0x8000 + ((PLAY_MASK - (PlayVal) + 1) & PLAY_MASK);
    NRF_PWM0->TASKS_SEQSTART[0] = 1;
    NRF_TIMER1->EVENTS_COMPARE[0] = 0;
    (void)(NRF_TIMER1->EVENTS_COMPARE[0]);
}

void Play_Init(void){
    /* Timer1 configure */
    NRF_TIMER1->MODE = 0; // Timer Mode
    NRF_TIMER1->BITMODE = 0;// 16 Bit
    NRF_TIMER1->PRESCALER = 4;
    NRF_TIMER1->CC[0] = 125;//8K
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->SHORTS = 1 << 0;
    NRF_TIMER1->INTENSET = 1 << 16;

    NRF_PWM0->PSEL.OUT[0] = (PWM0_OUT_PIN << 0) | (0 << 31);
    NRF_PWM0->PSEL.OUT[2] = (PWM1_OUT_PIN << 0) | (0 << 31);

    NRF_PWM0->ENABLE = (1 << 0);
    NRF_PWM0->MODE = (0 << 0);
    NRF_PWM0->COUNTERTOP = (1<<PLAY_BITS);
    NRF_PWM0->PRESCALER = (2 << 0);;

    NRF_PWM0->DECODER = (1 << 0) | (0 << 8);
    NRF_PWM0->LOOP = (0 << 0);

    NRF_PWM0->SEQ[0].PTR = (unsigned int)(PWM_Duty);
    NRF_PWM0->SEQ[0].CNT = 2;
    NRF_PWM0->SEQ[0].REFRESH = 0;
    NRF_PWM0->SEQ[0].ENDDELAY = 0;
    
    PlayState = 0;
}

void Play_Start(void){
    
    if ( PlayState == 0){
        PlayState = 1;
        
        //Kill IO config to avoid noise between play and stop.
        //NRF_P0->PIN_CNF[PWM0_OUT_PIN] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
        //NRF_P0->PIN_CNF[PWM1_OUT_PIN] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
      
        NRF_TIMER1->TASKS_START = 1;
        NVIC_EnableIRQ(TIMER1_IRQn);
    }
}

void Play_Stop(void){  
    if ( PlayState == 1){
        PlayState = 0;
      
        NRF_PWM0->TASKS_STOP = 1;
      
        //Kill IO config to avoid noise.
        //NRF_P0->OUTCLR = 1 << PWM0_OUT_PIN; //set PWM PIN as high level
        //NRF_P0->OUTCLR = 1 << PWM1_OUT_PIN;

        NRF_TIMER1->TASKS_STOP = 1;
        NRF_TIMER1->TASKS_CLEAR = 1;
        NVIC_DisableIRQ(TIMER1_IRQn);

        //Kill IO config to avoid noise.
        //NRF_P0->PIN_CNF[PWM0_OUT_PIN] = (0 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);
        //NRF_P0->PIN_CNF[PWM1_OUT_PIN] = (0 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);      
    }
}

void Play_EventTips( const short tips[], uint32_t len, float volume ){
    if (0 == ReceiverSC.TipsPlayPointer) {
        if(ReceiverSC.eventInfo.timeout < SystemTick_Get()) {
            ReceiverSC.eventInfo.timeout = SystemTick_Get() + 5000;
            Play_SetParam(SAMPLE_RATE_16000,ADC_BITS_14,FPWM_DIV_1);
            ReceiverSC.TipsPlayPointer = (unsigned int)tips;
            ReceiverSC.TipsPlayLen = len;
            ReceiverSC.PlayVol = volume;
            NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
            NRF_CLOCK->TASKS_HFCLKSTART = 1;
            while(0 == NRF_CLOCK->EVENTS_HFCLKSTARTED);
            Play_Start();
        }else{
            NRF_CLOCK->TASKS_HFCLKSTOP = 1;
        }
    }  
}

void Play_ShutDown(void){
    /* stop and reset var */
    Play_Stop();
    ReceiverSC.PlayBufPlayPos = 0;
    ReceiverSC.PlayBufRecvPos = 0;
    ReceiverSC.TipsPlayPointer = 0;
    ReceiverSC.TipsPlayLen = 0;
    ReceiverSC.TotalFrames = 0;
}

void Play_SetParam(unsigned char rate,unsigned char adcbit,unsigned char fpwm_div){

    Play_ShutDown();
    
    switch(rate){
        case SAMPLE_RATE_22050:{
            NRF_TIMER1->PRESCALER = 1;
            NRF_TIMER1->CC[0] = 363;//22.308
        }break;
        case SAMPLE_RATE_11025:{
            NRF_TIMER1->PRESCALER = 2;
            NRF_TIMER1->CC[0] = 363;//11019
        }break;
        case SAMPLE_RATE_16000:{
            NRF_TIMER1->PRESCALER = 3;
            NRF_TIMER1->CC[0] = 125;//16000
        }break;
        default:{
            NRF_TIMER1->PRESCALER = 4;
            NRF_TIMER1->CC[0] = 125;//8000
        }break;
    }
    switch(adcbit){
        case ADC_BITS_10:{
            NRF_PWM0->COUNTERTOP = 256;
        }break;
        case ADC_BITS_12:{
            NRF_PWM0->COUNTERTOP = 256;
        }break;
        case ADC_BITS_14:{
            NRF_PWM0->COUNTERTOP = (1<<PLAY_BITS);
        }break;
        case ADC_BITS_16:{
            NRF_PWM0->COUNTERTOP = 1024;
        }break;
        default :{
            NRF_PWM0->COUNTERTOP = 1024;
        }break;
    }
    switch(fpwm_div){
        case FPWM_DIV_1:{
            NRF_PWM0->PRESCALER = 0;
        }break;
        case FPWM_DIV_4:{
            NRF_PWM0->PRESCALER = 2;
        }break;
        case FPWM_DIV_8:{
            NRF_PWM0->PRESCALER = 3;
        }break;
        case FPWM_DIV_16:{
            NRF_PWM0->PRESCALER = 4;
        }break;
        default :{
            NRF_PWM0->PRESCALER = 1;
        }break;
    }
}

unsigned short Play_GetState(void)
{
    return PlayState;
}
