#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "Sender.h"
#include "Queue.h"
#include "Flash.h"
#include "Twis.h"
#include "Led.h"
#include "At.h"

#ifdef AT_FUNCTIONS

//#define AT_DEBUG

#if defined(AT_DEBUG) && defined(SERIAL_DEBUG)
#define AtLog(...) printf(__VA_ARGS__)
#else
#define AtLog(...)
#endif

bool simCReg = false;

const ATCmd AtSet[] = { 
    {"AT", "OK", 4},             //Synchronizate UART rate
    {"ATE0", "OK", 1},           //Cancel ECHO
    {"AT+SLEDS=1,1000,0", "OK", 1},  //Set LED on when SIM800 isn't registered to network 
    {"AT+CREG?", "OK", 20},      //Check GSM register.
    {"AT+CSCLK=2", "OK", 1},     //Enable slow clock automatically
    {"ATS0=3", "OK", 1},         //Auto answer after received 3 rings
    {"AT+CRSL=40", "OK", 1},     //Set RING sound level
    {"AT+CLVL=80", "OK", 1},     //Set SPEAKER sound level
    {"AT+CMIC=0,15", "OK",1}     //Set main audio mic gain
};

void SIM800C_PowerOn( bool on )
{
    bool simPowerStatus = false;  
    
    AtLog("Power...");
    if ( 0 != (NRF_P0->IN & (1 << SIM800C_STATE_PIN))){
        simPowerStatus = true;				
    }else{
        simPowerStatus = false;				
    }
				
    if ( simPowerStatus != on ){
        SenderSC.sim800State = SIM800_POWER_GOON;
        NRF_P0->OUTSET = (1 << (SIM800_POWER_PIN)); 
        Delay_ms(500);
   
        NRF_P0->OUTCLR = (1 << (SIM800_POWER_PIN)); 
        Delay_ms(2000);
        NRF_P0->OUTSET = (1 << (SIM800_POWER_PIN)); 
					
        simPowerStatus = on;
    }
				
    if ( simPowerStatus ){
			  uint32_t counter = 0;
        Delay_ms(1500);
        while( counter < 100 && 0 == (NRF_P0->IN & (1 << SIM800C_STATE_PIN))){ 
            Delay_ms(100);
					  counter++;
        }
				
				if ( counter < 100 ){
            AT_SimInit();
				}else{
					  simPowerStatus = false;
				}
    }
				
    //Update state
    SenderSC.sim800State = (simPowerStatus?SIM800_ON:SIM800_OFF);
				
    AtLog("800C %s\n", simPowerStatus?"on":"off");
}

void AT_CmdSend( char *cmd )         
{
    //Send AT command
    printf("%s\r\n", cmd);
}

/********************
 * AT command.
 */
void AT_Command(char *cmd, char *response, uint8_t wait_time)         
{
    bool waitCREG = false;

    if ( strcmp(cmd, "AT+CREG?") == 0 && SenderSC.csmCardIn){
        waitCREG = true;		
    }
		
    do{
        int count = 15;
        
        //Send AT command
        AT_CmdSend(cmd);
      
        //Check AT response.
        while(count--){
            Delay_ms(100);
            if(GetMessage() == ATMSG_RESPONSE) 
            {
                uint8_t msgBuf[UART_FRAME_SIZE]={0};
                Frame msg;
                msg.size = 0;
                msg.data = msgBuf;
                pullQueue(&uartQ, &msg);
                if ( msg.size ){
                    char *qstr;
                    if (!waitCREG){
                        if ( strstr((char*)msg.data, response ) !=0 ){
                            wait_time = 0;
                            break;
                        }
                    }
										
                    if ( (qstr = strstr((char*)msg.data, "+CREG:" )) !=0 ){
                         if ( qstr[9] == '1' ||	qstr[9] == '5'){
                             waitCREG = false;
                             simCReg = true;													 
                         }													 
                    }			
                }                
            }
        }
    }while( wait_time-- > 0);
}

/**
 * @brief SIM card configuring
 */
void AT_SimInit(void)
{
    int i;
    int cmdNum = sizeof(AtSet)/sizeof(ATCmd);  

    Uart_Init(AT_UART_RX_PIN, AT_UART_TX_PIN);
  
    simCReg = false;
	
    for(i=0;i<cmdNum;i++){
        AT_Command(AtSet[i].cmd, AtSet[i].response, AtSet[i].wait_time);      
    }

#ifdef SERIAL_DEBUG	
    Uart_Init(UART_RX_PIN, UART_TX_PIN);   
#endif
    AtLog("CSMINS:%d CREG:%d\n", SenderSC.csmCardIn, simCReg);		
}

bool SimCardDetect(void){
    static uint32_t lastTM = 0;
    uint32_t currentTM = SystemTick_GetSecond();
  
    if ( lastTM != currentTM ){
        lastTM = currentTM;
        if ( SenderSC.nRF52State == NRF52_ON && SenderSC.sim800State != SIM800_POWER_GOON ){
            //Check mic insert state
            bool detectResult = false; 
            if ( ( NRF_GPIO->IN & (1<<SIMCARD_DETECT_PIN)) != 0 ){
                detectResult = true;
            }				
						
            if ( SenderSC.csmCardIn != detectResult ){
                 return true;
            }
        }
    }		

    return false;		
}

void SimCardUpdate(void)
{
    if(( NRF_GPIO->IN & (1<<SIMCARD_DETECT_PIN)) != 0 ){
        SenderSC.csmCardIn = true;
    }else{
        SenderSC.csmCardIn = false;
    }				
}

#endif
