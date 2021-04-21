#include "Receiver.h"

#define RFX2411N 0
#if RFX2411N
#define RFX2411N_PIN_TXEN   (21)
#define RFX2411N_PIN_RXEN   (20)
#define RFX2411N_PIN_MODE   (19)
#define RFX2411N_PIN_SWANT  (18)
#endif

void Rf_Init(unsigned int TxAddr){

#if RFX2411N
    NRF_P0->PIN_CNF[RFX2411N_PIN_TXEN] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
    NRF_P0->PIN_CNF[RFX2411N_PIN_RXEN] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
    NRF_P0->PIN_CNF[RFX2411N_PIN_MODE] = (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);
    NRF_P0->PIN_CNF[RFX2411N_PIN_SWANT]= (1 << 0) | (0 << 1) | (3 << 2) | (3 << 8) | (0 << 16);

    NRF_P0->OUTCLR = 1 << RFX2411N_PIN_MODE;
    NRF_P0->OUTSET = 1 << RFX2411N_PIN_TXEN;
    NRF_P0->OUTCLR = 1 << RFX2411N_PIN_RXEN;
#endif
    NRF_RADIO->SHORTS = (1 << 1) | (1 << 4);//END_DISABLE ADDRESS_RSSISTART
    NRF_RADIO->TXPOWER = 0x04;  //4dbm
    NRF_RADIO->MODE = 0x00;     //1Mbps
    NRF_RADIO->PCNF0 = 0x00;    //set S0 LENGTH S1 of length as 0
    NRF_RADIO->PCNF1 = (0xFF << 0) | ((DATALEN_PER_FRAME+6) << 8) | (2 << 16) | (1 << 24) | (1 << 25);
    NRF_RADIO->CRCCNF = (1 << 0) | (0 << 8);
    NRF_RADIO->CRCINIT = 0xFFFFUL;
    NRF_RADIO->CRCPOLY = 0x11021UL;

    NRF_RADIO->PREFIX0 = 0x0000005A | ((TxAddr >> 24) << 8);
    NRF_RADIO->BASE0 = 0x00A5A55A;
    NRF_RADIO->BASE1 = TxAddr & 0x00FFFFFF;
    /* TxAddress0(0x5AA5A55A) is broadcast address */
    NRF_RADIO->TXADDRESS = 0x00;
    NRF_RADIO->RXADDRESSES = 0x01;

    NRF_RADIO->INTENSET = (1 << 3);//EVENTS_END
    NVIC_EnableIRQ(RADIO_IRQn);
}

void RADIO_IRQHandler(void){

    NRF_RADIO->EVENTS_END = 0;
    (void)(NRF_RADIO->EVENTS_END);
    if( NRF_RADIO->RXADDRESSES & (1 << NRF_RADIO->RXMATCH) ){
        ReceiverSC.FlagRfRecv = 1;
    }
}

void Rf_EnterScanMode(char scan){
    static char oldscan = (char)-1;	
    if ( oldscan != scan ){
        oldscan = scan;
        /* restart RF module */
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE = 1;
        while (0 == NRF_RADIO->EVENTS_DISABLED);    //MUST wait for disabled
        NRF_RADIO->RXADDRESSES = (scan) ? 0x01 : 0x02;
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_RXEN = 1;  //From diable->enable,may cost 140+6 us
        while(0 == NRF_RADIO->EVENTS_READY);
    }
}

int Rf_SendPackage(unsigned int bufPointer, unsigned int len, unsigned char channel) { 

    NRF_RADIO->TASKS_STOP = 1;
#if RFX2411N
    NRF_P0->OUTCLR = 1 << RFX2411N_PIN_RXEN;
    NRF_P0->OUTSET = 1 << RFX2411N_PIN_TXEN;
#endif    
    if ((channel != ((NRF_RADIO->FREQUENCY) & 0x7F)) || (NRF_RADIO->STATE < 9) || (NRF_RADIO->STATE > 11)) {
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE = 1;
        while (0 == NRF_RADIO->EVENTS_DISABLED);    //MUST wait for disabled
        NRF_RADIO->FREQUENCY = (channel % 100)|(FREQ_RANGE << 8);
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
#if RFX2411N
    NRF_P0->OUTCLR = 1 << RFX2411N_PIN_RXEN;
    NRF_P0->OUTCLR = 1 << RFX2411N_PIN_TXEN;
#endif
    return 0;
}

void Rf_BondResponse(unsigned int bufPointer,
                     unsigned int len,
                     unsigned char channel){
    char *dev = (char*)&ReceiverSC.deviceId;
    char *p = (char*)bufPointer;

    p[19] |= 0x02; //Device type: RECEIVER
    if( ReceiverSC.state == SYSTEM_STATE_BONDED){
        p[19] |= 0x04; //Bond end
    }else{
        p[19] &= ~0x04;//Bonding
    }
	
    //For bond command.
    p[20] = dev[0];
    p[21] = dev[1];
    p[22] = dev[2];
    p[23] = dev[3];
    
    Rf_SendPackage(bufPointer, 24, channel);
}

void Rf_StartRx(unsigned int bufPointer,unsigned int len,unsigned char channel) { 	
	  
    NRF_RADIO->TASKS_STOP = 1;

  	Rf_EnterScanMode(ReceiverSC.Pair.WorkChannel==channel?0:1);
#if RFX2411N
    NRF_P0->OUTSET = 1 << RFX2411N_PIN_RXEN;
    NRF_P0->OUTCLR = 1 << RFX2411N_PIN_TXEN;
#endif
    if ((channel != ((NRF_RADIO->FREQUENCY) & 0x7F)) || (NRF_RADIO->STATE < 1) || (NRF_RADIO->STATE > 3)) {
        /* restart RF module */
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE = 1;
        while (0 == NRF_RADIO->EVENTS_DISABLED);    //MUST wait for disabled
        NRF_RADIO->FREQUENCY = (channel % 100) | (FREQ_RANGE << 8);
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

void Rf_Stop(void){

    NRF_RADIO->TASKS_STOP = 1;
    NRF_RADIO->TASKS_DISABLE = 1;
    NRF_RADIO->INTENCLR = (1 << 3);//EVENTS_END
    NVIC_DisableIRQ(RADIO_IRQn);    
}
