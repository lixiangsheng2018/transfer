#include "stdbool.h"
#include "nrf_assert.h"
#include "Sender.h"
#include "STransfer.h"
#include "At.h"
#include "Led.h"

//#define BAT_DEBUG
#define PLAY_NOW                0

#if PLAY_NOW
#define PWM0_OUT_PIN            (19)
#define PWM1_OUT_PIN            (18)
#define PWM_COUNTERTOP          (1024)

unsigned short PWM_Duty[] = {0x8080,0x8080};
unsigned int PlayPos = 0;
#endif

#ifdef BAT_DEBUG
#define BatLog(...) printf(__VA_ARGS__)
#else
#define BatLog(...)
#endif

BattGrade BatteryGrade = BATT_GRADE_LOW;
uint32_t meanVal = 0;

__IO bool sampleState = 0;

static short SAADC_SampleBuffer[SAADC_CHANNEL_NUMBER];

void TIMER1_IRQHandler(void){

    signed short * pBuffer = (signed short *)(SenderSC.SampleBufPointer); 

    if ( sampleState == 0 ) return;

    /* force change to positive voltage */
    if (SAADC_SampleBuffer[0] < 0) SAADC_SampleBuffer[0] = 0;
    int SampleValue = (SAADC_SampleBuffer[0]<<2) + 1365;    
    
    pBuffer[SenderSC.SampleCurPos % SenderSC.SampleBufLen] = (signed short)(SampleValue - 32768);
    SenderSC.SampleCurPos++;

#if PLAY_NOW
    signed short val = 0;
    if ( PlayPos < SenderSC.SampleSendPos ) {
        val = pBuffer[PlayPos % SenderSC.SampleBufLen];
        PlayPos++;
    }

    val = (val + 32768) >> 6;
		
    PWM_Duty[0] = 0x8000 + ((val)&0x3FF); 
    PWM_Duty[1] = 0x8000 + ((1023 - (val)) & 0x3FF);
    NRF_PWM0->TASKS_SEQSTART[0] = 1;
#endif
	
    NRF_SAADC->TASKS_STOP = 1;
    NRF_SAADC->TASKS_START = 1;
    NRF_SAADC->TASKS_SAMPLE = 1;

    NRF_TIMER1->EVENTS_COMPARE[0] = 0;
    (void)(NRF_TIMER1->EVENTS_COMPARE[0]);
}

void Sample_SetSrc( SAMPLE_SRC src){
#if (HARD_BOARD == BOARD_6)  
    switch ( src ){
    case SSRC_NOISE:{
    #ifdef NOISE_SAMPLE_SWITCH  
        NRF_P0->OUTCLR = (1 << (NOISE_SAMPLE_SWITCH)); 
    #endif  
    }break;
    case SSRC_MIC:{
    #ifdef NOISE_SAMPLE_SWITCH  
        NRF_P0->OUTSET = (1 << (NOISE_SAMPLE_SWITCH)); 
    #endif  
    #ifdef AUDIO_SAMPLE_SWITCH  
        NRF_P0->OUTSET = (1 << (AUDIO_SAMPLE_SWITCH));  
    #endif
    }break;
    case SSRC_DATA:{  
    #ifdef NOISE_SAMPLE_SWITCH  
        NRF_P0->OUTSET = (1 << (NOISE_SAMPLE_SWITCH)); 
    #endif  
    #ifdef AUDIO_SAMPLE_SWITCH  
        NRF_P0->OUTCLR = (1 << (AUDIO_SAMPLE_SWITCH));  
    #endif  
    }break;
    default:{
         Log("Don't support sample source.\n");
    }break;
    }
#else  
    switch ( src ){
    case SSRC_NOISE:{
    #ifdef NOISE_SAMPLE_SWITCH  
        NRF_P0->OUTCLR = (1 << (NOISE_SAMPLE_SWITCH)); 
    #endif  
    #ifdef ADC_SAMPLE_MIC  
        NRF_SAADC->CH[0].PSELP = ADC_SAMPLE_MIC + 1;        
    #endif      
    }break;
    case SSRC_MIC:{
    #ifdef NOISE_SAMPLE_SWITCH  
        NRF_P0->OUTSET = (1 << (NOISE_SAMPLE_SWITCH));  
    #endif
    #ifdef ADC_SAMPLE_MIC      
        NRF_SAADC->CH[0].PSELP = ADC_SAMPLE_MIC + 1;
    #endif
    }break;
    case SSRC_DATA:{  
    #ifdef NOISE_SAMPLE_SWITCH  
        NRF_P0->OUTSET = (1 << (NOISE_SAMPLE_SWITCH));  
    #endif  
        NRF_SAADC->CH[0].PSELP = ADC_SAMPLE_DATA + 1;
    }break;
    default:{
         Log("Don't support sample source.\n");
    }break;
    }
#endif    
}

void Battery_Init( void ){
#ifdef ADC_SAMPLE_BATTERY
    NRF_SAADC->CH[1].PSELN = 0;
    NRF_SAADC->CH[1].PSELP = ADC_SAMPLE_BATTERY + 1;
    NRF_SAADC->CH[1].CONFIG = (0 << 0) | (0 << 4) | (0 << 8) | (0 << 12) | (0 << 16) | (0 << 20) | (0 << 24);
#endif
#ifdef BATTERY_LEVEL_DETECT
    BatteryGrade = BATT_GRADE_LOW;
#else
    BatteryGrade = BATT_GRADE_UNKNOWN;
#endif  
}

void Sample_Init(void){
    /* Timer1 configure */
    NRF_TIMER1->MODE = 0; // Timer Mode
    NRF_TIMER1->BITMODE = 0;// 16 Bit
    switch(SenderSC.Pair.Rate){
        case SAMPLE_RATE_11025:{
            NRF_TIMER1->PRESCALER = 2;
            NRF_TIMER1->CC[0] = 363;//11019
        }break;
        case SAMPLE_RATE_16000:{
            NRF_TIMER1->PRESCALER = 3;
            NRF_TIMER1->CC[0] = 125;//16000
        }break;
        case SAMPLE_RATE_22050:{
            NRF_TIMER1->PRESCALER = 1;
            NRF_TIMER1->CC[0] = 363;//22.308
        }break;
        default:{
            NRF_TIMER1->PRESCALER = 4;
            NRF_TIMER1->CC[0] = 125;//8000
        }break;
    }
    NRF_TIMER1->TASKS_CLEAR = 1;
    NRF_TIMER1->SHORTS = 1 << 0;
    NRF_TIMER1->INTENSET = 1 << 16;

    /* ADC configure: 0.6V internel reference voltage, VDD is 3.3V, gain = 1/6 */
#ifdef NOISE_SAMPLE_SWITCH  
    NRF_P0->PIN_CNF[NOISE_SAMPLE_SWITCH] = (1 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);
#endif    
#ifdef AUDIO_SAMPLE_SWITCH  
    NRF_P0->PIN_CNF[AUDIO_SAMPLE_SWITCH] = (1 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);
#endif    
#ifdef AUDIO_SRC_SELECT_PIN  
    NRF_P0->PIN_CNF[AUDIO_SRC_SELECT_PIN] = (0 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);
#endif    
    NRF_SAADC->CH[0].PSELN = 0;
    NRF_SAADC->CH[0].PSELP = ADC_SAMPLE_DATA + 1;
    NRF_SAADC->CH[0].CONFIG = (0 << 0) | (0 << 4) | (0 << 8) | (0 << 12) | (0 << 16) | (0 << 20) | (0 << 24);

    Battery_Init();
		    
    switch(SenderSC.Pair.AdcBits){
        case ADC_BITS_10:{
            NRF_SAADC->RESOLUTION = 1;
        }break;
        case ADC_BITS_12:{
            NRF_SAADC->RESOLUTION = 2;
        }break;
        case ADC_BITS_14:{
            NRF_SAADC->RESOLUTION = 3;
        }break;
        case ADC_BITS_16:{
            NRF_SAADC->RESOLUTION = 3;
        }break;
        default :{
            NRF_SAADC->RESOLUTION = 0;
        }break;
    }
    NRF_SAADC->SAMPLERATE = (0 << 0) | (0 << 12);
    NRF_SAADC->RESULT.PTR = (unsigned int)(SAADC_SampleBuffer);
    NRF_SAADC->RESULT.MAXCNT = SAADC_CHANNEL_NUMBER;
    NRF_SAADC->ENABLE = 1;
    
    NRF_SAADC->TASKS_STOP = 1;
    NRF_SAADC->TASKS_START = 1;
    NRF_SAADC->TASKS_SAMPLE = 1;
    
#if PLAY_NOW
    NRF_P0->PIN_CNF[PWM0_OUT_PIN] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
    NRF_P0->PIN_CNF[PWM1_OUT_PIN] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
    NRF_PWM0->PSEL.OUT[0] = (PWM0_OUT_PIN << 0) | (0 << 31);
    NRF_PWM0->PSEL.OUT[2] = (PWM1_OUT_PIN << 0) | (0 << 31);

    NRF_PWM0->ENABLE = (1 << 0);
    NRF_PWM0->MODE = (0 << 0);
    NRF_PWM0->COUNTERTOP = (PWM_COUNTERTOP << 0);
    NRF_PWM0->PRESCALER = (0 << 0);

    NRF_PWM0->DECODER = (1 << 0) | (0 << 8);
    NRF_PWM0->LOOP = (0 << 0);
    
    NRF_PWM0->SEQ[0].PTR = (unsigned int)PWM_Duty;
    NRF_PWM0->SEQ[0].CNT = 2;
    NRF_PWM0->SEQ[0].REFRESH = 0;
    NRF_PWM0->SEQ[0].ENDDELAY = 0;
#endif

    return;
}

void Sample_Start(void){
    if ( sampleState == false){
        sampleState = true;
        
        TransferReset();        

        NRF_TIMER1->TASKS_START = 1;
        NVIC_EnableIRQ(TIMER1_IRQn);
    }
    
    return;
}

void Sample_Stop(void){
    if ( sampleState == true ){
        sampleState = false;
        NRF_TIMER1->TASKS_STOP = 1;
        NVIC_DisableIRQ(TIMER1_IRQn);
    }
}

void NoiseSample_Start(void){
    //Select noise sample
    Sample_SetSrc(SSRC_NOISE);
    Sample_Start();
    return;
}

void NoiseSample_Stop(void){    
    Sample_Stop();
    //Dis-select noise sample
    Sample_SetSrc(SSRC_DATA);
}

void MicStateUpdate(void)
{
#ifdef AUDIO_SRC_SELECT_PIN      
    if ( ( NRF_GPIO->IN & ((uint32_t)1<<AUDIO_SRC_SELECT_PIN)) == 0 ){
        Sample_SetSrc(SSRC_MIC);
        SenderSC.micIn = true;
    }else{
        Sample_SetSrc(SSRC_DATA);
        SenderSC.micIn = false;
    }            
#endif 
#ifdef BLUETOOTH_POWER_PIN
    if( SenderSC.nRF52State == NRF52_OFF || SenderSC.micIn){        
        NRF_P0->OUTCLR = (1 << (BLUETOOTH_POWER_PIN)); 
    }else{
        NRF_P0->OUTSET = (1 << (BLUETOOTH_POWER_PIN)); 
    }
#endif 
}

void Noise_Init(void)
{
#if (DENOISE_PROCESS != DENOISE_WEBRTC)
    NoiseSample_Start();
    /* we get noise to calc out noise level and use for deniose */
    while (SenderSC.SampleCurPos < (DATALEN_PER_RAWFRAME * NOISE_SAMPLE_FREAME_NUMBER));
    NoiseSample_Stop();
#endif  
    SignalProcess_Init((signed short *)SenderSC.SampleBufPointer,NOISE_SAMPLE_FREAME_NUMBER);
}

bool AudioLineIsChanged(void){
#ifdef AUDIO_SRC_SELECT_PIN  
    static uint32_t lastTM = 0;
    uint32_t currentTM = SystemTick_GetSecond();
  
    if ( lastTM != currentTM ){
        lastTM = currentTM;
        if ( SenderSC.nRF52State == NRF52_ON && SenderSC.sim800State != SIM800_POWER_GOON ){
            //Check mic insert state
            bool micState = false; 
            if ( ( NRF_GPIO->IN & ((uint32_t)1<<AUDIO_SRC_SELECT_PIN)) == 0 ){
                micState = true;
            }        
            
            if ( SenderSC.micIn != micState ){
                return true;
            }
        }
    }    
#endif    
    return false;    
}

BattGrade BatteryLevelDetect(void){    
#ifdef BATTERY_LEVEL_DETECT
    static uint32_t lastTM = 0;
    static uint32_t ledTM = 0;
  
    uint32_t curTM = SystemTick_Get();
  
    //Update battery level every 1000ms.
    if ( lastTM + 1000 < curTM ){
        static uint8_t id = 0;
        static uint8_t samples = 0;
        static uint8_t lowCount = 0;
        static uint8_t midLowCount = 0;
        static uint8_t warningCount = 0;
        static uint16_t batteryLevel[4];
			
        lastTM = curTM;      
        
        ASSERT( SAADC_CHANNEL_NUMBER > 1 );  
      
        if ( sampleState == false ){
            //If sample is stopped, start a battery level sample.
            NRF_SAADC->EVENTS_STOPPED = 0;
            NRF_SAADC->TASKS_STOP = 1; 
            while (0 == NRF_SAADC->EVENTS_STOPPED) ;           
         
            NRF_SAADC->EVENTS_END = 0;
            NRF_SAADC->TASKS_START = 1;
            NRF_SAADC->TASKS_SAMPLE = 1;
            while (0 == NRF_SAADC->EVENTS_END) ; 
        }

        if ( SAADC_SampleBuffer[1] < 0 ) return BATT_GRADE_UNKNOWN;
				
        batteryLevel[id++%4] = SAADC_SampleBuffer[1];
        if ( samples < 3 ) {
            meanVal = batteryLevel[samples];
            samples++;
        }else{        
            meanVal = (batteryLevel[0] + batteryLevel[1] + batteryLevel[2] + batteryLevel[3])>>2;          
        }
        
        if ( meanVal < BATTERY_LOW_THRESHOLD ){
            if ( lowCount > 5 ){
                BatteryGrade = BATT_GRADE_LOW;
            }else{
                lowCount++;
            }
            if (midLowCount<5) midLowCount++;
            if (warningCount<5) warningCount++;
        }else if ( meanVal < BATTERY_MID_LOW_THRESHOLD ){
            lowCount = 0;
            if(midLowCount > 5 ){
                BatteryGrade = BATT_GRADE_MID_LOW;
            }else{
                midLowCount++;
            }
            if (warningCount<5) warningCount++;
        }else if ( meanVal < BATTERY_WARNING_THRESHOLD ){
            lowCount = 0;
            midLowCount = 0;
            if(warningCount > 5 ){
                BatteryGrade = BATT_GRADE_WARNING;
            }else{
                warningCount++;
            }
        }else{
            lowCount = 0;
            midLowCount = 0;
            warningCount = 0;
            if (meanVal<BATTERY_GRADE_1_THRESHOLD){
                BatteryGrade = BATT_GRADE_1;
            }else if (meanVal<BATTERY_GRADE_2_THRESHOLD){
                BatteryGrade = BATT_GRADE_2;
            }else{
                BatteryGrade = BATT_GRADE_3;
            }
        }

        BatLog("Bat:%d %d %d\n", meanVal, BatteryGrade, warningCount);       
    }
    
    //Update LED status every 100ms
    if ( ledTM + 100 < curTM ){
        static uint16_t ledCount = 0;
        static LedId runningId = STATE_LED;
        LedId led = STATE_LED;
        LedState ledState = (SenderSC.micIn?LED_STATE_ON:LED_STATE_OFF);
        ledTM = curTM;

        if( SenderSC.nRF52State == NRF52_OFF || 
           ( BatteryGrade > BATT_GRADE_WARNING && SenderSC.sim800State != SIM800_POWER_GOON ) || 
           ( BatteryGrade <= BATT_GRADE_WARNING && led != BATLEVEL_IND_LED )){
            if ( runningId != STATE_LED ){  
                BatLog("Restore led %d to STATE led\n", led);
                SetLedState( runningId, LED_STATE_OFF );
                runningId = STATE_LED;											
            }
        }
				
        ledCount++;
        if ( SenderSC.nRF52State == NRF52_OFF ){
            ledState = LED_STATE_OFF;
        }else{
            switch( BatteryGrade ){
            case BATT_GRADE_WARNING:
            case BATT_GRADE_MID_LOW:
            case BATT_GRADE_LOW:{ 
                if ( (BatteryGrade != BATT_GRADE_WARNING && ledCount>100) || ledCount>200){
                    ledCount = 0;
                    led = BATLEVEL_IND_LED;
                    ledState = LED_STATE_ON;
                    SenderSC.eventInfo.newEvent = AUDIO_EVENT_LOWPOWER;
                    Log("Battery ind %d\n", (uint16_t)BatteryGrade);
                }
            }break;
            default:
                ledCount = 0;
                if( SenderSC.sim800State == SIM800_POWER_GOON ) {
                    //Power on, running horse led.
                    SetLedState( led, LED_STATE_OFF );
                    if ( runningId == STATE_LED){
                        runningId = CHARGING_LED;
                    }else if ( runningId == CHARGING_LED ){
                        runningId = STANDBY_LED;
                    }else{
                        runningId = STATE_LED;
                    }
                    led = runningId;
                    ledState = LED_STATE_ON;
                }
            }
        }
				
        SetLedState( led, ledState );
    }
#endif

    return BatteryGrade;
}

BattGrade BatteryGetGrade(void)
{
    return BatteryGrade;
}

uint16_t BatteryGetValue(void)
{
    return (uint16_t)meanVal;
}
