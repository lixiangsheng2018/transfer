#include "Sender.h"
#include "STransfer.h"
#include "Flash.h"

#define RFX2411N     0
#define RFX2402E     1
#if (HARD_BOARD == BOARD_6 || HARD_BOARD == BOARD_7)
#define PA_MODULE    RFX2402E 
#else
#define PA_MODULE    RFX2411N
#endif

void PAM_Init(void)
{
#ifdef PA_PIN_TXEN  
    NRF_P0->PIN_CNF[PA_PIN_TXEN] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);  
#endif  
#ifdef PA_PIN_RXEN  
    NRF_P0->PIN_CNF[PA_PIN_RXEN] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
#endif  
#ifdef PA_PIN_MODE  
    NRF_P0->PIN_CNF[PA_PIN_MODE] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
#endif  
#ifdef PA_PIN_SWANT  
    NRF_P0->PIN_CNF[PA_PIN_SWANT]= (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
#endif  

#ifdef PA_PIN_PDET  
    NRF_P0->PIN_CNF[PA_PIN_PDET]= (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
#endif  

#ifdef PA_PIN_POWER  
    NRF_P0->PIN_CNF[PA_PIN_POWER]= (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
#endif  
  
#ifdef PA_PIN_MODE  
    NRF_P0->OUTCLR = 1 << PA_PIN_MODE; 
#endif  
#ifdef PA_PIN_RXEN  
    NRF_P0->OUTCLR = 1 << PA_PIN_RXEN;
#endif  
#ifdef PA_PIN_TXEN  
    NRF_P0->OUTCLR = 1 << PA_PIN_TXEN;
#endif    
#ifdef PA_PIN_PDET  
    NRF_P0->OUTSET = 1 << PA_PIN_PDET;
#endif  
#ifdef PA_PIN_POWER  
    NRF_P0->OUTCLR = 1 << PA_PIN_POWER;
#endif  
}

void PAM_Power( bool bOn ){
#ifdef PA_PIN_POWER
   if (bOn) NRF_P0->OUTSET = 1 << PA_PIN_POWER;
   else NRF_P0->OUTCLR = 1 << PA_PIN_POWER;
#endif  
}

void PAM_Enable( RF_TYPE mask )
{
#ifdef PA_PIN_RXEN
    if ( mask & RFT_RX ){
        NRF_P0->OUTSET = 1 << PA_PIN_RXEN;
    }else{
        NRF_P0->OUTCLR = 1 << PA_PIN_RXEN;
    }
#endif  
#ifdef PA_PIN_TXEN
    if ( mask & RFT_TX ){
        NRF_P0->OUTSET = 1 << PA_PIN_TXEN;
    }else{
        NRF_P0->OUTCLR = 1 << PA_PIN_TXEN;
    }
#endif  
}

void Rf_Init(void){
    //Initialize PA module.
    PAM_Init();
  
    NRF_RADIO->SHORTS = (1 << 1) | (1 << 4);//END_DISABLE ADDRESS_RSSISTART
    NRF_RADIO->TXPOWER = 0x04;  //4dbm
    NRF_RADIO->MODE = 0x00;     //1Mbps
    NRF_RADIO->PCNF0 = 0x00;    //Set the length of S0 S1 as 0, LENGTH's as 8 Bits. 
    NRF_RADIO->PCNF1 = (0xFF << 0) | ((DATALEN_PER_FRAME+6) << 8) | (2 << 16) | (1 << 24) | (1 << 25); 
    NRF_RADIO->CRCCNF = (1 << 0) | (0 << 8);
    NRF_RADIO->CRCINIT = 0xFFFFUL;
    NRF_RADIO->CRCPOLY = 0x11021UL;

    NRF_RADIO->PREFIX0 = 0x0000005A | ((SenderSC.txAddress >> 24) << 8);
    NRF_RADIO->BASE0 = 0x00A5A55A;
    NRF_RADIO->BASE1 = SenderSC.txAddress & 0x00FFFFFF;
    /* TxAddress0(0x5AA5A55A) is broadcast address */
    NRF_RADIO->TXADDRESS = 0x01;   //Select logic address 1 to send data
    NRF_RADIO->RXADDRESSES = 0x01; //Select logic address 0 to receive reponse information

    NRF_RADIO->INTENSET = (1 << 3);//EVENTS_END
    NVIC_EnableIRQ(RADIO_IRQn);		
}

void RADIO_IRQHandler(void){  
    PAM_Enable(RFT_NONE);

    NRF_RADIO->EVENTS_END = 0;
    (void)(NRF_RADIO->EVENTS_END);
    if( NRF_RADIO->RXADDRESSES & (1 << (NRF_RADIO->RXMATCH))){
        SenderSC.FlagRfRecv = 1;
    }    
}

void Rf_AddressUpdate(void)
{
	  uint32_t newAddress = (SenderSC.deviceInfo.addressId?*((unsigned int *)0x7FFF8):*((unsigned int *)0x7FFFC));
	  if ( SenderSC.txAddress != newAddress){
        SenderSC.txAddress = newAddress;
	
        NRF_RADIO->TASKS_STOP = 1;
        NRF_RADIO->PREFIX0 = 0x0000005A | ((SenderSC.txAddress >> 24) << 8);
        NRF_RADIO->BASE1 = SenderSC.txAddress & 0x00FFFFFF;
        memcpy(SenderSC.Pair.PairHeader + 6, &SenderSC.txAddress ,4);
	
        UpdateDevicesInfo();
		}
}

void Rf_ReceiveUpdate(void)
{
    uint8_t *p;

    if (SYSTEM_STATE_SCAN == SenderSC.state) {
        /* scan used channel now */
        if (0 == memcmp((void *)SenderSC.RfBufPointer, (void *)SenderSC.Pair.PairHeader,6)) {
            p = (uint8_t *)(SenderSC.RfBufPointer + 14);
            /* this channel was used */
            SenderSC.Pair.UsedChannelBitMaps[(*p)/32] |= 1 << ((*p)%32);
            NRF_RADIO->TASKS_START = 1; // continue to scan
        }
    }
    else if ( SYSTEM_STATE_BONDING == SenderSC.state ) {
        /* Check channel if matched */
        if (0 == memcmp((void *)SenderSC.RfBufPointer, (void *)SenderSC.Pair.PairHeader,10)){
					  uint32_t id = 0;
            char *dev = (char *)&id;
            char *src = (char *)SenderSC.RfBufPointer;    
            bool bFinish = ((src[19]&0x04)?true:false);
							
            if( (src[19] & 0x02 )	!= 0x02 ) {
                //Only receive a message from RECEIVER.
                NRF_RADIO->TASKS_START = 1; // continue to scan
                return;
            }
						
            dev[0] = src[10];
            dev[1] = src[11];
            dev[2] = src[12];
            dev[3] = src[13];
            p = (uint8_t *)(SenderSC.RfBufPointer + 18);
            if ((*p == SenderSC.brdcstType) && 
                (*p == BROADCAST_BOND || *p == BROADCAST_UNBOND || *p == BROADCAST_UNBOND_FORCE ) && 
                (id == SenderSC.deviceInfo.deviceId)){
                //Enter bond state and save connected device id.
                SenderSC.state = SYSTEM_STATE_BOND;
                SenderSC.ScanChannel.Broadcasts = 45;
                dev = (char*)&SenderSC.connectedDeviceId;
                dev[0] = src[20];               
                dev[1] = src[21];               
                dev[2] = src[22];               
                dev[3] = src[23]; 
                if ( *p == BROADCAST_BOND ){
                    SenderSC.brdcstType = BROADCAST_BOND_DEVICE;
                }else{
                    SenderSC.brdcstType = BROADCAST_UNBOND_DEVICE;
                }									
                Log("Bond:%d %x\n", *p, SenderSC.connectedDeviceId);
            }else if((*p == SenderSC.brdcstType ) && 
                     (*p == BROADCAST_BOND_DEVICE || *p == BROADCAST_UNBOND_DEVICE ) && 
                     (id == SenderSC.deviceInfo.deviceId) ){
                dev[0] = src[20];               
                dev[1] = src[21];               
                dev[2] = src[22];               
                dev[3] = src[23]; 
                if ( id == SenderSC.connectedDeviceId && bFinish ){
                    SenderSC.state = SYSTEM_STATE_BONDED;
                    Log("Bonded:%d %x\n", *p, id);
                }                     
            }else{
                NRF_RADIO->TASKS_START = 1; // continue to scan
            }
        }
    }
}

void Rf_Broadcast( uint32_t bufPointer, uint32_t len, uint8_t channel, uint8_t type, uint32_t dstDevice ){
    int i;
    char * p = ( char *)bufPointer;
    char * dev = (char*)&SenderSC.deviceInfo.deviceId;
    memcpy(p, (void *)SenderSC.Pair.PairHeader, 10);
    p += 10;
    
    *p++ = dev[0];
    *p++ = dev[1];
    *p++ = dev[2];
    *p++ = dev[3];
    *p++ = SenderSC.Pair.WorkChannel;
    *p++ = SenderSC.Pair.Rate;
    *p++ = SenderSC.Pair.AdcBits;
    *p++ = SenderSC.Pair.FpwmDiv;
    *p++ = type;
    *p++ = (SenderSC.deviceInfo.datatype||SenderSC.deviceInfo.bondedNum==0?1:0);
  
    if ( type == BROADCAST_BOND_DEVICE ||
         type == BROADCAST_UNBOND_DEVICE ){
        dev  = (char*)&dstDevice;
        *p++ = dev[0];
        *p++ = dev[1];
        *p++ = dev[2];
        *p++ = dev[3];
    }else{
        for ( i=0; i<8; i++ ){
            dev  = (char*)&SenderSC.deviceInfo.bondedDevices[i];
            *p++ = dev[0];
            *p++ = dev[1];
            *p++ = dev[2];
            *p++ = dev[3];
        } 
    }
    
    len = p - ( char *)bufPointer;
#if (TRANSFER_MODE == TRANSFER_AFTER_PROCESSED)
    NRF_RADIO->TXADDRESS = 0x00;
    Rf_SendPackage(bufPointer, len, channel);
    NRF_RADIO->TXADDRESS = 0x01;
#else
    broadcastProcess((char*)bufPointer, len, channel);
#endif
}

void Rf_StartRx(uint32_t bufPointer, uint32_t len, uint8_t channel) { 

    NRF_RADIO->TASKS_STOP = 1;
    PAM_Enable(RFT_RX);
  
    if ((channel != ((NRF_RADIO->FREQUENCY) & 0x7F)) || (NRF_RADIO->STATE < 1) || (NRF_RADIO->STATE > 3)) {
        /* restart RF module */
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE = 1;
        while (0 == NRF_RADIO->EVENTS_DISABLED);    //MUST wait for disabled
        NRF_RADIO->FREQUENCY = (channel % 101)|(FREQ_RANGE << 8);
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_RXEN = 1;  //from diable->enable,may task 140+6 us
        while(0 == NRF_RADIO->EVENTS_READY);
    }
    
    NRF_RADIO->PCNF1 = (NRF_RADIO->PCNF1&~RADIO_PCNF1_STATLEN_Msk) | ((len>DATALEN_PER_FRAME+6?DATALEN_PER_FRAME+6:len) << RADIO_PCNF1_STATLEN_Pos);
    NRF_RADIO->PACKETPTR = bufPointer;
    NRF_RADIO->TASKS_START = 1;
    NRF_RADIO->EVENTS_CRCERROR = 0;
    NRF_RADIO->EVENTS_CRCOK = 0;
    NRF_RADIO->INTENSET = (1 << 3);//EVENTS_END
    NRF_RADIO->EVENTS_END = 0;

    NVIC_EnableIRQ(RADIO_IRQn);
}

int Rf_SendPackage(uint32_t bufPointer, uint32_t len, uint8_t channel) { 

    NRF_RADIO->TASKS_STOP = 1;
    PAM_Enable(RFT_TX);
  
    if ((channel != ((NRF_RADIO->FREQUENCY) & 0x7F)) || (NRF_RADIO->STATE < 9) || (NRF_RADIO->STATE > 11)) {
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE = 1;
        while (0 == NRF_RADIO->EVENTS_DISABLED);    //MUST wait for disabled
        NRF_RADIO->FREQUENCY = (channel % 101)|(FREQ_RANGE << 8);
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_TXEN = 1;  //from diable->enable,may task 140+6 us
        while(0 == NRF_RADIO->EVENTS_READY);
    }
    NRF_RADIO->PCNF1 = (NRF_RADIO->PCNF1&~RADIO_PCNF1_STATLEN_Msk) | ((len>DATALEN_PER_FRAME+6?DATALEN_PER_FRAME+6:len) << RADIO_PCNF1_STATLEN_Pos);
    NRF_RADIO->PACKETPTR = bufPointer;
    
    NRF_RADIO->EVENTS_END  = 0U;
    NRF_RADIO->INTENCLR = (1 << 3); //EVENTS_END
    NRF_RADIO->TASKS_START = 1U;
    while(0 == NRF_RADIO->EVENTS_END);
    NRF_RADIO->EVENTS_END = 0;
    
    PAM_Enable(RFT_NONE);

    return 0;
}

void Rf_SetMode(int mode)
{
#ifdef PA_PIN_SWANT
    if ( mode ) {
        NRF_P0->OUTSET = 1 << PA_PIN_SWANT;
    } else {
        NRF_P0->OUTCLR = 1 << PA_PIN_SWANT;        
    }
#endif               
}

void Rf_TestStart( uint8_t Channel, bool Dir, uint32_t Duration){
    char szBuffer[12];  
    Frame uartFrame;

    sprintf(szBuffer, "rftest %d %d %d", (uint32_t)Channel, (int)Dir, Duration );
    uartFrame.size = strlen(szBuffer) + 1 ;
    uartFrame.data = (uint8_t*)szBuffer;
    pushQueue(&uartQ, &uartFrame);
    PostMessage( MSG_UART_RX );
}

bool Rf_Test(void)
{
    static uint8_t channel = 0;
    static bool dir = false;

    if( SenderSC.rft.channel != channel || SenderSC.rft.dir != dir ){
        channel = SenderSC.rft.channel; 
        dir = SenderSC.rft.dir;			
				if ( SenderSC.rft.duration == (uint32_t)-1 ){
					  SenderSC.ScanChannel.ScanTimeout = SenderSC.rft.duration;
				}else{					
            SenderSC.ScanChannel.ScanTimeout = SystemTick_Get() + SenderSC.rft.duration;
				}
				
        Sample_Stop();		

        if ((channel != ((NRF_RADIO->FREQUENCY) & 0x7F)) || (NRF_RADIO->STATE < 9) || (NRF_RADIO->STATE > 11)) {
            NRF_RADIO->EVENTS_DISABLED = 0;
            NRF_RADIO->TASKS_DISABLE = 1;
            while (0 == NRF_RADIO->EVENTS_DISABLED);    //MUST wait for disabled
            NRF_RADIO->FREQUENCY = (channel % 101)|(FREQ_RANGE << 8);
            NRF_RADIO->EVENTS_READY = 0;
            NRF_RADIO->TASKS_TXEN = 1;  //from diable->enable,may task 140+6 us
            while(0 == NRF_RADIO->EVENTS_READY);
        }

        PAM_Enable(RFT_TX);
        Rf_SetMode(dir);
        Log("Rf_Test:%d %d %d\n", channel, dir, SenderSC.rft.duration);				
    }

    if ( SenderSC.state > SYSTEM_STATE_INITED &&
         SenderSC.ScanChannel.ScanTimeout < SystemTick_Get() ){
        SenderSC.rft.duration = 0;			
    }
         
    return SenderSC.rft.duration?true:false;
}
