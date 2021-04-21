#include "stdbool.h"
#include "Sender.h"
#include "Led.h"
#include "nrf_delay.h"

//#define LED_DEBUG

#if defined(LED_DEBUG) && defined(SERIAL_DEBUG)
#define LedLog(...) printf(__VA_ARGS__)
#else
#define LedLog(...)
#endif

LedPin ledPins[LED_NUM];
uint8_t BatIndCount = 0; 
uint8_t LedFlashCount = 0; 
ChargeState chargingState;

void Led_Init( void )
{
#ifdef LED0_OUT_PIN
    ledPins[0].pin = LED0_OUT_PIN;
    ledPins[0].state = LED_STATE_OFF;
    //POUT with pull-down and output 0
    NRF_P0->PIN_CNF[LED0_OUT_PIN] = (1 << 0) | (0 << 1) | (0 << 2) | (0 << 8) | (0 << 16);
    LED_OFF(LED0_OUT_PIN);
#endif
#ifdef LED1_OUT_PIN
    ledPins[1].pin = LED1_OUT_PIN;
    ledPins[1].state = LED_STATE_OFF;
    NRF_P0->PIN_CNF[LED1_OUT_PIN] = (1 << 0) | (0 << 1) | (0 << 2) | (0 << 8) | (0 << 16);
    LED_OFF(LED1_OUT_PIN);
#endif
#ifdef LED2_OUT_PIN
    ledPins[2].pin = LED2_OUT_PIN;
    ledPins[2].state = LED_STATE_OFF;
    NRF_P0->PIN_CNF[LED2_OUT_PIN] = (1 << 0) | (0 << 1) | (0 << 2) | (0 << 8) | (0 << 16);
    LED_OFF(LED2_OUT_PIN);
#endif
  
#ifdef CHARGING_PIN
    NRF_P0->PIN_CNF[CHARGING_PIN] = (0 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);
#endif
#ifdef STANDBY_PIN
    NRF_P0->PIN_CNF[STANDBY_PIN] = (0 << 0) | (0 << 1) | (3 << 2) | (0 << 8) | (0 << 16);
#endif
    chargingState = CHARGE_STATE_OFF;
}

void SetLedState( LedId led, LedState state )
{
    static bool ledState = false;
    if( led == BATLEVEL_IND_LED ){
        LedLog("Batt Ind:%d\n", state);
        if ( state == LED_STATE_ON ){
            BatIndCount = LED_BATIND_DURATION;
            LedFlashCount = 0; 
        }else{
            BatIndCount = 1;      
        }
    }else{
        ledPins[led].state = state;     
    }
    
    if (BatIndCount){
        BatIndCount--;  
        if ( BatIndCount == 0 || LedFlashCount >= LED_FLASH_NUM ){
            LedFlashCount = 0;
            BatIndCount = 0;
            if ( SenderSC.micIn && (SenderSC.nRF52State == NRF52_ON) ){
                ledState = true;
                LED_ON(ledPins[STATE_LED].pin);
                LedLog("LED On\n");
            }else{
                ledState = false;
                LED_OFF(ledPins[STATE_LED].pin);
                LedLog("LED Off\n");
            }
            LED_OFF(ledPins[CHARGING_LED].pin);
            LED_OFF(ledPins[STANDBY_LED].pin);            
        }else{
            BattGrade batGrad = BatteryGetGrade();
            switch(batGrad){
            case BATT_GRADE_LOW:{
                if ( (BatIndCount%2) == 0){
                    ledState = false;
                    LED_OFF(ledPins[STATE_LED].pin);
                }else{
                    LED_ON(ledPins[STATE_LED].pin);
                    if ( ledState == false ) LedFlashCount++;
                    ledState = true;
                }
                LED_OFF(ledPins[CHARGING_LED].pin);
                LED_OFF(ledPins[STANDBY_LED].pin);            
                LedLog("BATT_GRADE_LOW: %d %d.\n", BatIndCount, LedFlashCount);
            }break;
            case BATT_GRADE_MID_LOW:{
                if (((BatIndCount>>1)%2)==0){
                    ledState = false;
                    LED_OFF(ledPins[STATE_LED].pin);
                    if ( LedFlashCount == 3){
                        LedFlashCount = LED_FLASH_NUM;
                    }
                }else{
                    LED_ON(ledPins[STATE_LED].pin);
                    if ( ledState == false ) LedFlashCount++;
                    ledState = true;
                }
                LED_OFF(ledPins[CHARGING_LED].pin);
                LED_OFF(ledPins[STANDBY_LED].pin);            
                LedLog("BATT_GRADE_MID_LOW: %d %d.\n", BatIndCount, LedFlashCount);
            }break;
            case BATT_GRADE_WARNING:{
                if (((BatIndCount>>2)%2)==0){
                    ledState = false;
                    LED_OFF(ledPins[STATE_LED].pin);
                    if ( LedFlashCount == 2){
                        LedFlashCount = LED_FLASH_NUM;
                    }
                }else{
                    LED_ON(ledPins[STATE_LED].pin);
                    if ( ledState == false ) LedFlashCount++;
                    ledState = true;
                }
                LED_OFF(ledPins[CHARGING_LED].pin);
                LED_OFF(ledPins[STANDBY_LED].pin);
                LedLog("BATT_GRADE_WARNING: %d %d.\n", BatIndCount, LedFlashCount);
            }break;
            case BATT_GRADE_1:{
                LedFlashCount++;
                LED_ON(ledPins[STATE_LED].pin);
                LED_OFF(ledPins[CHARGING_LED].pin);
                LED_OFF(ledPins[STANDBY_LED].pin);
                LedLog("BATT_GRADE_1: %d.\n", BatIndCount);
            }break;
            case BATT_GRADE_2:{
                LedFlashCount++;
                LED_ON(ledPins[STATE_LED].pin);
                LED_ON(ledPins[CHARGING_LED].pin);
                LED_OFF(ledPins[STANDBY_LED].pin);
                LedLog("BATT_GRADE_2: %d.\n", BatIndCount);
            }break;
            case BATT_GRADE_3:
            default:{
                LedFlashCount++;
                LED_ON(ledPins[STATE_LED].pin);
                LED_ON(ledPins[CHARGING_LED].pin);
                LED_ON(ledPins[STANDBY_LED].pin);
                LedLog("BATT_GRADE_3: %d.\n", BatIndCount);
            }break;
            }
        }        
    }else{      
    #if defined( CHARGING_PIN) && defined( STANDBY_PIN )
    #if ( HARD_BOARD == BOARD_5 || HARD_BOARD == BOARD_7)			
        if ( (NRF_P0->IN & (1<<CHARGING_PIN)) == 0 && (NRF_P0->IN & (1<<STANDBY_PIN)) == 0 ){
            chargingState = CHARGE_STATE_ON;   
        }else{
            chargingState = CHARGE_STATE_OFF;
        }
    #else
        if ( (NRF_P0->IN & (1<<CHARGING_PIN)) == 0 && (NRF_P0->IN & (1<<STANDBY_PIN))!= 0 ){
            chargingState = CHARGE_STATE_ON;   
        }else if ( (NRF_P0->IN & (1<<CHARGING_PIN)) != 0 && (NRF_P0->IN & (1<<STANDBY_PIN)) == 0 ){
            chargingState = CHARGE_STATE_FULL;          
        }else{
            chargingState = CHARGE_STATE_OFF;
        }
    #endif	
				
        if ( chargingState == CHARGE_STATE_OFF){
             //No charge
             LED_OFF(ledPins[CHARGING_LED].pin);          
             LED_OFF(ledPins[STANDBY_LED].pin);          
        }else if ( chargingState == CHARGE_STATE_ON){
            //Charging
            uint16_t battVal = BatteryGetValue();
            LedFlashCount++;
            if ( (LedFlashCount%5) == 4 ){
                if ( battVal < BATTERY_GRADE_1_THRESHOLD ){
                    LED_TOGGLE(ledPins[STATE_LED].pin);               
                    LED_OFF(ledPins[CHARGING_LED].pin);          
                    LED_OFF(ledPins[STANDBY_LED].pin);                         
                }else if (battVal < BATTERY_GRADE_2_THRESHOLD ){
                    LED_ON(ledPins[STATE_LED].pin);          
                    LED_TOGGLE(ledPins[CHARGING_LED].pin);               
                    LED_OFF(ledPins[STANDBY_LED].pin);          
                }else{
                    LED_ON(ledPins[STATE_LED].pin);          
                    LED_ON(ledPins[CHARGING_LED].pin);          
                    LED_TOGGLE(ledPins[STANDBY_LED].pin);               
                }
                LedLog("Charging:%d %d\n", battVal, SystemTick_Get());
            }
            
        #if (AUTO_POWERON_ON_CHARGING == 1)
            if( SenderSC.nRF52State == NRF52_OFF){
                PostMessage( MSG_POWER_KEY );
                Log("Power on\n");
            }
        #endif            
        }else{
            //Charge full
            LED_ON(ledPins[STATE_LED].pin);          
            LED_ON(ledPins[CHARGING_LED].pin);          
            LED_ON(ledPins[STANDBY_LED].pin);          
        }
    #endif  
        
        if ( chargingState == CHARGE_STATE_OFF ){
            if ( state == LED_STATE_OFF ) LED_OFF(ledPins[led].pin);
            if ( state == LED_STATE_ON ) LED_ON(ledPins[led].pin);
            if ( state == LED_STATE_FLASH ) LED_TOGGLE(ledPins[led].pin);
#if UNBOUND_FORCE	
            if ( state == LED_STATE_OFF && SenderSC.nRF52State==NRF52_ON) LED_ON(ledPins[STATE_LED].pin);
#endif					
					
        }        
    }
}

bool LedIsBusy( void ){
    return BatIndCount>0 || chargingState != CHARGE_STATE_OFF;
}
