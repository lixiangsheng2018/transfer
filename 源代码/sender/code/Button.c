#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "Sender.h"
#include "Queue.h"
#include "Flash.h"
#include "Twis.h"
#include "Led.h"
#include "At.h"

#ifdef BUTTON_FUNCTIONS

//#define BUTTON_DEBUG

#if defined(BUTTON_DEBUG) && defined(SERIAL_DEBUG)
#define BtnLog(...) printf(__VA_ARGS__)
#else
#define BtnLog(...)
#endif

//#define VOL_SMALL_STEP             
#ifndef VOL_SMALL_STEP
const unsigned char volSet[] = {MIN_VOLUME, MIN_VOLUME*2, MIN_VOLUME*4, MIN_VOLUME*8, MIN_VOLUME*16};
char volStep = 0;
#else
char volStep = 16;
#endif

static ButtonInfo pressedButton;

/***********************
 * @brief gpioe callback
 */
static void 
app_gpioe_cb(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    uint32_t input = NRF_GPIO->IN;
  
    if ( SenderSC.state == SYSTEM_STATE_POWER_OFF ){
        if ( pin != BUTTON_POWER_PIN ){
            return;
        }
    }
    
    switch( pin )
    {
        case BUTTON_POWER_PIN:
            if ( ( input & (1<<BUTTON_POWER_PIN)) == 0 ){
                pressedButton.pin = pin;    
                pressedButton.elapseTM = SystemTick_Get();
                pressedButton.bPressed = 1;
            }else{
                if ( pressedButton.bPressed && pressedButton.pin == BUTTON_POWER_PIN ){
                    SetLedState( BATLEVEL_IND_LED, LED_STATE_ON );                      
                }
                pressedButton.elapseTM = 0;
                pressedButton.bPressed = 0;
            }
            BtnLog("Power key:%d\n", pressedButton.bPressed);
            break;
        case BUTTON_ADD:
            if ( ( input & (1<<BUTTON_ADD)) == 0 ){
                if (0 == (input & ((uint32_t)1 << BUTTON_SELECT_PIN))){
                    //Adjust volume if possible.
                    if ( SenderSC.deviceInfo.volume < MAX_VOLUME ){
                    #ifndef VOL_SMALL_STEP
                        if ( volStep<sizeof(volSet)/sizeof(volSet[0])-1) volStep++;
                        SenderSC.deviceInfo.volume = volSet[volStep];
                    #else
                        if ( volStep<32) volStep++;
                        SenderSC.deviceInfo.volume += volStep;
                        if (SenderSC.deviceInfo.volume>MAX_VOLUME) SenderSC.deviceInfo.volume = MAX_VOLUME;
                    #endif
                        pressedButton.elapseTM = SystemTick_Get();
                        pressedButton.bPressed = 1;
                        pressedButton.interval = 500;                    
                        pressedButton.pin = pin;    
                        SenderSC.eventInfo.newEvent = AUDIO_EVENT_BUTTON;                                              
                    }else{
                        SetLedState( STATE_LED, SenderSC.micIn?LED_STATE_OFF:LED_STATE_ON );
                    }
                }else{
                    pressedButton.elapseTM = 0;
                    if ( SenderSC.deviceInfo.bondedNum < MAX_BONDED_DEVICES ){
                        //Bond.
                        pressedButton.pin = pin;    
                        Broadcast_SetType( BROADCAST_BOND, AUDIO_EVENT_RECONNECT );                    
                    }
                }
                BtnLog("ADD pressed. %d\n", SenderSC.deviceInfo.volume);
            }else if( pressedButton.pin == BUTTON_ADD ) {
                pressedButton.elapseTM = 0;
                pressedButton.bPressed = 0;
                SetLedState( STATE_LED, SenderSC.micIn?LED_STATE_ON:LED_STATE_OFF );
                BtnLog("Add released.\n");
            }
            break;
        case BUTTON_MINUS:
            if ( ( input & (1<<BUTTON_MINUS)) == 0){
                if (0 == (input & ((uint32_t)1 << BUTTON_SELECT_PIN))){
                    if( SenderSC.deviceInfo.volume > MIN_VOLUME ){
                    #ifndef VOL_SMALL_STEP
                        if ( volStep ) volStep--;
                        SenderSC.deviceInfo.volume = volSet[volStep];
                    #else
                        if ( volStep>16) volStep--;
                        SenderSC.deviceInfo.volume -= volStep;
                        if (SenderSC.deviceInfo.volume<MIN_VOLUME) SenderSC.deviceInfo.volume=MIN_VOLUME;
                    #endif
                        pressedButton.elapseTM = SystemTick_Get();
                        pressedButton.bPressed = 1;
                        pressedButton.interval = 500;                    
                        pressedButton.pin = pin;    
                        SenderSC.eventInfo.newEvent = AUDIO_EVENT_BUTTON;
                    }else{
                        SetLedState( STATE_LED, SenderSC.micIn?LED_STATE_OFF:LED_STATE_ON );
                    }
                }else{
                    pressedButton.elapseTM = 0;
                    //Unbond device
                    pressedButton.pin = pin; 
                #if (UNBOUND_FORCE == 0)	
                    Broadcast_SetType( BROADCAST_UNBOND, AUDIO_EVENT_RECONNECT );
                #else
                    Broadcast_SetType( BROADCAST_UNBOND_FORCE, AUDIO_EVENT_RECONNECT );
                #endif
                }                
                BtnLog("Minus pressed.%d\n", SenderSC.deviceInfo.volume);
            }else if( pressedButton.pin == BUTTON_MINUS ) {
                pressedButton.elapseTM = 0;
                pressedButton.bPressed = 0;
                SetLedState( STATE_LED, SenderSC.micIn?LED_STATE_ON:LED_STATE_OFF );
                BtnLog("Minus released.\n");
            }
            break;
    }
}
#endif

/**
 * @brief Function for configuring: PIN_IN pin for input, PIN_OUT pin for output, 
 * and configures GPIOTE to give an interrupt on pin change.
 */
void BSP_GpioInit(void)
{
#ifdef BUTTON_FUNCTIONS  
    //PIN with pull-up
    NRF_P0->PIN_CNF[BUTTON_SELECT_PIN] = (0 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);  
#ifdef BLUETOOTH_POWER_PIN
    //Set BLUETOOTH power pin to LOW.
    NRF_P0->PIN_CNF[BLUETOOTH_POWER_PIN] = (1 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);
    NRF_P0->OUTCLR = (1 << (BLUETOOTH_POWER_PIN)); 
#endif  
#endif  
#ifdef AT_FUNCTIONS  
    NRF_P0->PIN_CNF[SIM800C_STATE_PIN] = (0 << 0) | (0 << 1) | (0 << 2) | (0 << 8) | (0 << 16);
    NRF_P0->PIN_CNF[SIMCARD_DETECT_PIN]= (0 << 0) | (0 << 1) | (0 << 2) | (0 << 8) | (0 << 16);
    SimCardUpdate();
  
    //Init SIM800C power-key.
    NRF_P0->PIN_CNF[SIM800_POWER_PIN] = (1 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);    
    NRF_P0->OUTSET = (1 << (SIM800_POWER_PIN)); 
#endif

#if( defined(BUTTON_FUNCTIONS) ||defined(AT_FUNCTIONS))
  
    //Initialize GPIOE
    ret_code_t err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

#ifdef BUTTON_FUNCTIONS  
    err_code = nrf_drv_gpiote_in_init(BUTTON_POWER_PIN, &in_config, app_gpioe_cb);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(BUTTON_POWER_PIN, true);     
        
    err_code = nrf_drv_gpiote_in_init(BUTTON_ADD, &in_config, app_gpioe_cb);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(BUTTON_ADD, true);     

    err_code = nrf_drv_gpiote_in_init(BUTTON_MINUS, &in_config, app_gpioe_cb);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(BUTTON_MINUS, true);     
#endif
    
#ifdef AT_FUNCTIONS  
    err_code = nrf_drv_gpiote_in_init(AT_UART_RI_PIN, &in_config, app_gpioe_cb);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(AT_UART_RI_PIN, true);
#endif
#endif    
}

void MessageProcess(Message msgId )
{  
    switch(msgId){
    case MSG_LOW_BATTERY:{
        SenderSC.nRF52State = NRF52_ON;
    }
    case MSG_POWER_KEY:{
        SenderSC.nRF52State = (SenderSC.nRF52State==NRF52_OFF?NRF52_ON:NRF52_OFF);               
        MicStateUpdate();      
#ifdef AT_FUNCTIONS
        SimCardUpdate();

        if ( SenderSC.nRF52State == NRF52_ON && !SenderSC.micIn ){
            SIM800C_PowerOn(true);
        }else{
            SIM800C_PowerOn(false);
        }      
#endif  
        if( SenderSC.nRF52State == NRF52_OFF ){
            //nRF52832 enter low power mode.
            SenderSC.state = SYSTEM_STATE_POWER_OFF;
            Sample_Stop();
            PAM_Power(false);
            //Save parameter.
            if(TWIS_VolumeUpdate()){
                UpdateDevicesInfo();
            }
        }else{
            SenderSC.state = SYSTEM_STATE_INITED;
            PAM_Power(true);
        }
#if 0        
        if ( SenderSC.nRF52State==NRF52_ON ){
            NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
            NRF_CLOCK->TASKS_HFCLKSTART = 1;
            while(0 == NRF_CLOCK->EVENTS_HFCLKSTARTED);
        }else{
            NRF_CLOCK->TASKS_HFCLKSTOP = 1;
        }        
#endif        
    }break;
    case MSG_SIMCARD_DETECT:
    case MSG_AUDIO_LINE_CHANGED:{
        MicStateUpdate();    
#ifdef AT_FUNCTIONS      
        SimCardUpdate();
      
        Sample_Stop();
        emptyQueue(&audioQ);

        if ( SenderSC.nRF52State == NRF52_ON && !SenderSC.micIn ){
            SIM800C_PowerOn(true);
        }else{
            SIM800C_PowerOn(false);
        }
        
        if ( SenderSC.nRF52State != NRF52_OFF ){
            if ( SenderSC.state > SYSTEM_STATE_PREPARE_WORK ){
                SenderSC.state = SYSTEM_STATE_PREPARE_WORK;
            }else{
                SenderSC.state = SYSTEM_STATE_INITED;
            }              
        }
#endif        
    }break;
    case MSG_UART_RX:{
        uint8_t msgBuf[UART_FRAME_SIZE]={0};
        Frame msg;
        msg.size = 0;
        msg.data = msgBuf;
        pullQueue(&uartQ, &msg);
        if ( msg.size ){
            char *word = strtok((char*)msg.data, " ");
            if ( word != NULL ){
                if ( strcmp(word, "bond") == 0 ){
                    if ( SenderSC.deviceInfo.bondedNum < MAX_BONDED_DEVICES ){
                        //Bond.
                        Broadcast_SetType( BROADCAST_BOND, AUDIO_EVENT_RECONNECT );                    
                    }
                }else if ( strcmp(word, "unbond") == 0 ){
                    word = strtok(NULL, " ");
                    if ( word == NULL || word[0] == '0' ){
                        //Unbond those receiver bonded by the sender.
                        Broadcast_SetType( BROADCAST_UNBOND, AUDIO_EVENT_RECONNECT );                    
                    }else{
                        //Unbond forcely.
                        Broadcast_SetType( BROADCAST_UNBOND_FORCE, AUDIO_EVENT_RECONNECT );                    
                    }
                }else if ( strcmp(word, "rftest") == 0 ){
                    SenderSC.rft.duration = (uint32_t)-1; 
                    SenderSC.rft.channel = SenderSC.ScanChannel.ScanChannel[0];
                    SenderSC.rft.dir = true;
                    word = strtok(NULL, " ");
                    if ( word != NULL) SenderSC.rft.channel = strtol(word, NULL, 0);
                    word = strtok(NULL, " ");
                    if ( word != NULL) SenderSC.rft.dir = strtol(word, NULL, 0)?true:false;
                    word = strtok(NULL, " ");
                    if ( word != NULL) SenderSC.rft.duration = strtol(word, NULL, 0);
                    
                    SenderSC.rft.state = SenderSC.state;
                    SenderSC.state = SYSTEM_STATE_RF_TEST;
                }else if ( strcmp(word, "dump") == 0 ){
                    word = strtok(NULL, " ");
                    if ( word != NULL){ 
                        if ( strcmp(word, "twis") == 0 ){
                            uint16_t reg = 0xff;
                            word = strtok(NULL, " ");
                            if ( word != NULL ){
                                reg = atoi(word);													   
                            }
                            TWIS_Dump((TWIS_REG)reg);
                        }else if ( strcmp(word, "devinfo") == 0 ){
                            Log("Devices:%08x %d %08x %08x %08x %08x %08x %08x %08x %08x %d\n", 
													                        SenderSC.deviceInfo.addressId, 
													                        SenderSC.deviceInfo.bondedNum, 
													                        SenderSC.deviceInfo.bondedDevices[0],
													                        SenderSC.deviceInfo.bondedDevices[1],
													                        SenderSC.deviceInfo.bondedDevices[2],
													                        SenderSC.deviceInfo.bondedDevices[3],
													                        SenderSC.deviceInfo.bondedDevices[4],
													                        SenderSC.deviceInfo.bondedDevices[5],
													                        SenderSC.deviceInfo.bondedDevices[6],
													                        SenderSC.deviceInfo.bondedDevices[7],
													                        SenderSC.deviceInfo.volume);													
                        }												
                    }
                }    
            }
        }
    }break;
    default:
#ifdef BUTTON_FUNCTIONS
        if( pressedButton.bPressed ){
            uint32_t tm = SystemTick_Get();
            switch(pressedButton.pin){
            case BUTTON_POWER_PIN:{
                if ( tm - pressedButton.elapseTM > 1000 && pressedButton.elapseTM > 0 ){
                    if ( BatteryGetGrade() ){
                        SetLedState( STATE_LED, LED_STATE_ON );                  
                        PostMessage( MSG_POWER_KEY );
                        pressedButton.bPressed = 0;
                        BtnLog("Long Power key\n");                  
                    }else{
                        SetLedState( BATLEVEL_IND_LED, LED_STATE_ON );                      
                    }
                }
            }break;
#ifdef VOL_SMALL_STEP             
            case BUTTON_ADD:{
                if ( tm - pressedButton.elapseTM > pressedButton.interval ){
                    if ( SenderSC.deviceInfo.volume < MAX_VOLUME ){
                        if(SenderSC.deviceInfo.volume + volStep <= MAX_VOLUME){
                            SenderSC.deviceInfo.volume+= volStep;
                        }else{
                            SenderSC.deviceInfo.volume = MAX_VOLUME;
                        }
                        if ( volStep < 32 ) volStep++;
                        pressedButton.interval = 100;                    
                        pressedButton.elapseTM = tm;
                        SenderSC.eventInfo.updated = true;
                        BtnLog("Add updated %d.\n", SenderSC.deviceInfo.volume);
                    }else{
                        SetLedState( STATE_LED, SenderSC.micIn?LED_STATE_OFF:LED_STATE_ON );
                        BtnLog("volume = %d\n", SenderSC.deviceInfo.volume);
                    }
                }
            }break;
            case BUTTON_MINUS:{
                if ( tm - pressedButton.elapseTM > pressedButton.interval ){
                    if ( SenderSC.deviceInfo.volume > MIN_VOLUME  ){
                        if ( SenderSC.deviceInfo.volume - volStep > MIN_VOLUME ){
                            SenderSC.deviceInfo.volume-=volStep;
                        }else{
                            SenderSC.deviceInfo.volume = MIN_VOLUME;
                        }
                        if ( volStep < 32 ) volStep++;
                        pressedButton.interval = 100;
                        pressedButton.elapseTM = tm;
                        SenderSC.eventInfo.updated = true;
                        BtnLog("Minus updated %d.\n", SenderSC.deviceInfo.volume);
                    }else{
                        SetLedState( STATE_LED, SenderSC.micIn?LED_STATE_OFF:LED_STATE_ON );
                        BtnLog("volume = %d\n", SenderSC.deviceInfo.volume);
                    }
                }
            }break;
#endif            
            }
        }
#endif
        break;
    }

    
    if ( SenderSC.nRF52State == NRF52_OFF && LedIsBusy() == false ){
        __WFI();
    }
}
