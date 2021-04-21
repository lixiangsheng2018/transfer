#include "Receiver.h"
#include "RTransfer.h"
#include "Flash.h"
#include "stdbool.h"

//#define TIPS_TEST        

static signed short PlayBuffer[IMA_ADPCM_PCM_RAW_LEN * 80];
static unsigned char RfSendRecvBuffer[256];

ReceiverSysCfg ReceiverSC;

void GPIO_Reset(void){
    int i;
    for ( i = 0; i<32; i++){
        //Set all GPIOs to LOW.
        NRF_P0->PIN_CNF[i] = (1 << 0) | (0 << 1) | (0 << 2) | (3 << 8) | (0 << 16);
        NRF_P0->OUTCLR = 1 << i;
    }
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

void ReceiverSC_Init(void){

    memset(&ReceiverSC, 0x00, sizeof(ReceiverSC));
    ReceiverSC.PlayBufPointer = (unsigned int)PlayBuffer;
    ReceiverSC.PlayBufLen = sizeof(PlayBuffer)/sizeof(PlayBuffer[0]);
    ReceiverSC.PlayBufPlayPos = 0;
    ReceiverSC.PlayBufRecvPos = 0;
    ReceiverSC.PlayVol = 1.0;
    
    ReceiverSC.state = SYSTEM_STATE_INITED;

    ReceiverSC.RfBufPointer = (unsigned int)RfSendRecvBuffer;
    ReceiverSC.RfBufLen = sizeof(RfSendRecvBuffer)/sizeof(RfSendRecvBuffer[0]);

#if (FREQ_RANGE == FREQ_HIGH)    
    ReceiverSC.ScanChannel.ScanChannel[0] = 88; 
    ReceiverSC.ScanChannel.ScanChannel[1] = 82; 
    ReceiverSC.ScanChannel.ScanChannel[2] = 94; 
#else
    ReceiverSC.ScanChannel.ScanChannel[0] = 3;
    ReceiverSC.ScanChannel.ScanChannel[1] = 23;
    ReceiverSC.ScanChannel.ScanChannel[2] = 33;
#endif
    ReceiverSC.ScanChannel.CurChannel = 0;

    ReceiverSC.Pair.PairHeader[0] = 0xA5;
    ReceiverSC.Pair.PairHeader[1] = 0x5A;
    ReceiverSC.Pair.PairHeader[2] = 'D';
    ReceiverSC.Pair.PairHeader[3] = 'M';
    ReceiverSC.Pair.PairHeader[4] = 0x5A;
    ReceiverSC.Pair.PairHeader[5] = 0xA5;
    memcpy(ReceiverSC.Pair.PairHeader + 6,(unsigned char *)0x7FFFC,4);
    
    UpdateDeviceInfo();
    
    NRF_POWER->DCDCEN = 1;
    __enable_irq();
        
    return;
}

/*
 *DeviceIdCheck
 *parameter: type - 0 -- User connect check
 *                  1 -- Test connect check
 *                  2 -- Check device whether listed in bonded devices list or not.
 */
bool DeviceIdCheck( uint8_t type, uint32_t srcDevice, uint8_t *dstDevices){
#if ( RECV_MODE & RECV_MODE_BOND )  
    bool retVal = false;

    if ( type < 2 ){
        if(( ReceiverSC.boundedDeviceId	== 0 && (type & 0x01) == 0x01 ) || 
           ( ReceiverSC.boundedDeviceId == srcDevice && srcDevice != 0 )){ 
            retVal = true;       
        }else{
            //Those devices which bonded with other device cannot work.
            retVal = false;       
        }
        //Log("Id:%02x %08x %d\n", (uint32_t)type, srcDevice, (uint32_t)retVal);
    }else{
        unsigned int dev, i;
        char *pDev = (char*)&dev;
        for(i=0; i<8; i++){
            pDev[0] = dstDevices[i*4+0];                
            pDev[1] = dstDevices[i*4+1];                
            pDev[2] = dstDevices[i*4+2];                
            pDev[3] = dstDevices[i*4+3];
            if ( dev == ReceiverSC.deviceId ){
                retVal = true;       
                break;
            }              
        }          
    }
    return retVal;    
#else
    return true;
#endif

}

void BroadcastDataProcess( uint32_t *srcDevice ){
	
    switch(ReceiverSC.state){
    case SYSTEM_STATE_BONDING:						
    case SYSTEM_STATE_PREPARE_SCAN:
    case SYSTEM_STATE_SCANTIMEOUT:
    case SYSTEM_STATE_SCAN:{
        if(0 == memcmp((void *)(ReceiverSC.RfBufPointer),(void *)(ReceiverSC.Pair.PairHeader),10)){
            uint8_t *src = (uint8_t *)(ReceiverSC.RfBufPointer + 10);
            uint8_t *dev = (uint8_t *)srcDevice;

            if( *((uint8_t *)(ReceiverSC.RfBufPointer + 19)) & 0x02 ){
                //Do nothing for message from RECEIVER.													
                break;
            }
											
            dev[0] = *src++;
            dev[1] = *src++;
            dev[2] = *src++;
            dev[3] = *src++;
            ReceiverSC.Pair.WorkChannel = *src++;
            ReceiverSC.Pair.Rate = *src++;
            ReceiverSC.Pair.AdcBits = *src++;
            ReceiverSC.Pair.FpwmDiv = *src++;
            uint8_t infoType = *src++;
            uint8_t type = *src++;                        
												
            /* recive SENDER signal,we can get work channel rate etc. */
            if ( infoType == BROADCAST_DEFAULT ){
                if ( DeviceIdCheck( type&0x01, *srcDevice, src ) ){
                    ReceiverSC.DataType = (type & 0x01);
                    Play_SetParam(SAMPLE_RATE_16000,ADC_BITS_14,FPWM_DIV_1);
                    ReceiverSC.TipsPlayPointer = (unsigned int)(Tips_Connect);
                    ReceiverSC.TipsPlayLen = sizeof_Tips_Connect;
                    ReceiverSC.PlayVol = BEEP_VOLUME;
                    ReceiverSC.state = SYSTEM_STATE_SCANED;

                    Play_Start();
                }
            }
#if ( RECV_MODE & RECV_MODE_BOND )  
						else if ( infoType == BROADCAST_BOND ){
                if ( ReceiverSC.boundedDeviceId == 0 ||  //No bonded device
                    (ReceiverSC.boundedDeviceId == *srcDevice && !DeviceIdCheck( 2, *srcDevice, src )) //Device isn't included in packages of sender
                   ){ 
                    Log("Bonding: %x %s CHN:%d\n", 
                         *srcDevice,
                         ReceiverSC.boundedDeviceId==0?"New":"Old", 
                         ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]);
                                        
                         ReceiverSC.Responses = 8;
                         ReceiverSC.state = SYSTEM_STATE_BOND;
                         ReceiverSC.Timeout = SystemTick_Get() + RND_Get()%16;
                }
            }else if ( infoType == BROADCAST_BOND_DEVICE ){
                uint32_t dstDevice = 0;
                dev = (uint8_t *)&dstDevice;
                dev[0] = *src++;
                dev[1] = *src++;
                dev[2] = *src++;
                dev[3] = *src++;
                Log("Bonded: %x %x CHN:%d\n", 
                     *srcDevice,
                     dstDevice, 
                     ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]);
               
                if ( dstDevice == ReceiverSC.deviceId ){ 
                    Log("Bonded.");

  							    ReceiverSC.Responses = 16;
                    ReceiverSC.state = SYSTEM_STATE_BONDED;
                    if ( ReceiverSC.boundedDeviceId == 0 ){
                        SetBondedDevice(*srcDevice);
                    }																	  
                    ReceiverSC.Timeout = SystemTick_Get() + RND_Get()%16;
                }
            }else if( infoType == BROADCAST_UNBOND || 
                      infoType == BROADCAST_UNBOND_FORCE ){
                if (((ReceiverSC.boundedDeviceId == *srcDevice || infoType == BROADCAST_UNBOND_FORCE ) && ReceiverSC.boundedDeviceId != 0 )||
						 	      ( ReceiverSC.boundedDeviceId == 0 && DeviceIdCheck( 2, *srcDevice, src )) ){
                    Log("Unbonding: %x CHN:%d\n", 
                         *srcDevice,
                          ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]);

                    ReceiverSC.Responses = 8;
                    ReceiverSC.state = SYSTEM_STATE_BOND;
                    ReceiverSC.Timeout = SystemTick_Get() + RND_Get()%16;
                }
            }else if ( infoType == BROADCAST_UNBOND_DEVICE ){
                uint32_t dstDevice = 0;
                dev = (uint8_t *)&dstDevice;
                dev[0] = *src++;
                dev[1] = *src++;
                dev[2] = *src++;
                dev[3] = *src++;
													
                Log("UnBonded: %x %x chn:%d\n", 
                    *srcDevice,
                    dstDevice, 
                    ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]);
                if ( dstDevice == ReceiverSC.deviceId && ReceiverSC.boundedDeviceId != 0){ 
                    Log("UnBonded end.\n");
                                        
                    ReceiverSC.Responses = 16;
                    ReceiverSC.state = SYSTEM_STATE_BONDED;

                    SetBondedDevice(0);

                    ReceiverSC.Timeout = SystemTick_Get() + RND_Get()%16;
                }
            } 
#endif						
        }                             
    }break;
    default:{
    }break;
    }
}

void DataEventProcess( PCMFrame *pInFrame, bool selfData ){
#if ( RECV_MODE & RECV_MODE_BOND )  
    static unsigned char  diffFrames = 0;
    uint32_t time = SystemTick_Get();	
	
    if ( (RECV_MODE & RECV_MODE_BOND) && ReceiverSC.boundedDeviceId == 0 && ReceiverSC.NextTipsTime < time ){
        Play_SetParam(SAMPLE_RATE_16000, ADC_BITS_14, FPWM_DIV_1);
        ReceiverSC.TipsPlayPointer = (unsigned int)Tips_Unbonded;
        ReceiverSC.TipsPlayLen = sizeof_Tips_Unbonded;               
        ReceiverSC.NextTipsTime = time + BOND_TIPS_INTERVAL;                     
        ReceiverSC.state = SYSTEM_STATE_SCANED;
        ReceiverSC.PlayVol = BOND_VOLUME;
        Play_Start();
    }else if (selfData){
        EAudioEvent event = AUDIO_EVENT_NONE;
        if ( NRF_RADIO->CRCSTATUS ){
            event = (EAudioEvent)(pInFrame->info.dtype.evt);
            if( ReceiverSC.DataType != pInFrame->info.dtype.mode ){
                if ( diffFrames > 5 ){
                    //When data type is changed, enter scan mode. Unbonded device can only process test data.
                    ReceiverSC.state = SYSTEM_STATE_INITED; 
                    ReceiverSC.eventInfo.event = AUDIO_EVENT_NONE;
                    diffFrames = 0;
                    return;
                }
                diffFrames++;
                Log("T:%02x %d %d %08x\n", pInFrame->info.value, diffFrames, ReceiverSC.DataType, ReceiverSC.boundedDeviceId);											
            }else{						
                diffFrames = 0;
            }            
        }
                    
        switch( event ){
#if ( RECV_MODE	& RECV_MODE_BUTTON )				
        case AUDIO_EVENT_BUTTON:{
            if ( ReceiverSC.eventInfo.event != AUDIO_EVENT_BUTTON ){
                ReceiverSC.state = SYSTEM_STATE_BUTTON_BEEP;
                ReceiverSC.eventInfo.timeout = SystemTick_Get();
                ReceiverSC.eventInfo.event = AUDIO_EVENT_BUTTON;
                Log("Key pressed.\n");
            }
        }break;
#endif					
#if ( RECV_MODE	& RECV_MODE_LOW_BATTERY )				
        case AUDIO_EVENT_LOWPOWER:{
            if ( ReceiverSC.eventInfo.event != AUDIO_EVENT_LOWPOWER &&
                 ReceiverSC.eventInfo.timeout < SystemTick_Get()){
                ReceiverSC.state = SYSTEM_STATE_LOWPOWER;
                ReceiverSC.eventInfo.timeout = SystemTick_Get();
                ReceiverSC.eventInfo.event = AUDIO_EVENT_LOWPOWER;
                Log("Low power.\n");
            }
        }break;
#endif				
        case AUDIO_EVENT_RECONNECT:{
            if ( ReceiverSC.eventInfo.event != AUDIO_EVENT_RECONNECT ){
                Play_SetParam(SAMPLE_RATE_16000,ADC_BITS_14,FPWM_DIV_1);
                ReceiverSC.NextTipsTime = SystemTick_Get() + 5000;
                ReceiverSC.state = SYSTEM_STATE_PREPARE_SCAN;
                ReceiverSC.eventInfo.event = AUDIO_EVENT_RECONNECT;
            }
        }break;
        default:{
            ReceiverSC.eventInfo.event = AUDIO_EVENT_NONE;
        }break;                                            
        }
    }
#endif   
}

int main(void){
    unsigned char * pChar;
    uint32_t srcDevice;
    uint32_t oldState = 0;
    uint32_t rssiLowCount = 0;
  
    GPIO_Reset();
    SystemClock_Init();
    ReceiverSC_Init();
    Uart_Init();
    SystemTick_Init();
    Rf_Init(*((unsigned int *)(0x7FFFC)));
    rxTransferInit();
    Play_Init();

    Log("Receiver Build @ %s %08x->%08x\r\n",__TIME__, ReceiverSC.deviceId, ReceiverSC.boundedDeviceId);
  
    while(SYSTEM_STATE_EXIT != ReceiverSC.state){
        if ( ReceiverSC.FlagRfRecv || oldState != ReceiverSC.state){
            //Log("S:%d R:%d Scan:%d\n",(int)ReceiverSC.state, (int)ReceiverSC.FlagRfRecv, NRF_RADIO->RXADDRESSES);      
            oldState = ReceiverSC.state;
        }
        
        if (ReceiverSC.FlagRfRecv) {
            ReceiverSC.FlagRfRecv = 0;
            switch(ReceiverSC.state){
                case SYSTEM_STATE_WAIT:{
                    ReceiverSC.state = SYSTEM_STATE_RECV;
                }break;
                default:{
                    BroadcastDataProcess( &srcDevice );
                }break;
            }
        }

        switch (ReceiverSC.state) {
            case SYSTEM_STATE_INITED:{
                Play_SetParam(SAMPLE_RATE_16000,ADC_BITS_14,FPWM_DIV_1);
                ReceiverSC.NextTipsTime = SystemTick_Get() + 5000;
                ReceiverSC.state = SYSTEM_STATE_PREPARE_SCAN;
            }break;
            case SYSTEM_STATE_PREPARE_SCAN:{
                Rf_StartRx(ReceiverSC.RfBufPointer, ReceiverSC.RfBufLen, ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]); 
                ReceiverSC.Timeout = SystemTick_Get() + 288;
                ReceiverSC.state = SYSTEM_STATE_SCAN;
            }break;
            case SYSTEM_STATE_SCAN:{
                __WFE();
                if (ReceiverSC.Timeout < SystemTick_Get()) {
                    ReceiverSC.Timeout = SystemTick_Get() + 288;
                    ReceiverSC.state = SYSTEM_STATE_SCANTIMEOUT;
                    /* disable RF */
                    Rf_Stop();
									
                    /* scan timeout, change to next channel */
                    ReceiverSC.ScanChannel.CurChannel = (ReceiverSC.ScanChannel.CurChannel+1)%MAX_CHANNEL_NO;
									
                    if ( ReceiverSC.NextTipsTime < SystemTick_Get()) {
                        ReceiverSC.NextTipsTime = SystemTick_Get() + 5000;
                        /* play unconnect notify */
                        if ( ReceiverSC.boundedDeviceId == 0 || (RECV_MODE & RECV_MODE_BOND) == 0 ){
                            ReceiverSC.TipsPlayPointer = (unsigned int)(Tips_Unconnect);
                            ReceiverSC.TipsPlayLen = sizeof_Tips_Unconnect;
                            ReceiverSC.PlayVol = BEEP_VOLUME;
                        }else{
                            ReceiverSC.TipsPlayPointer = (unsigned int)(Tips_Unconnect_Bond);
                            ReceiverSC.TipsPlayLen = sizeof_Tips_Unconnect_Bond;               
                            ReceiverSC.PlayVol = BOND_VOLUME;
                        }

                        Play_Start();
                    }
                }else{
                    if ( NRF_RADIO->STATE < 1 || NRF_RADIO->STATE > 3 ){
                        Log("Rf_StartRx %d channel %d tm %d\n", NRF_RADIO->STATE, ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel], ReceiverSC.Timeout - SystemTick_Get());
                        //Re-scan if RX is disabled.
                        Rf_StartRx(ReceiverSC.RfBufPointer, ReceiverSC.RfBufLen, ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]); 
                    }
                }
            }break;
            case SYSTEM_STATE_SCANTIMEOUT:{
                __WFE();
                if(0 == ReceiverSC.TipsPlayPointer){
                    NRF_CLOCK->TASKS_HFCLKSTOP = 1;
                    if(ReceiverSC.ScanChannel.CurChannel == MAX_CHANNEL_NO-1){
                        SystemTick_ForceSleepMs(284 + RND_Get());
                    }
                }
                if(ReceiverSC.Timeout < SystemTick_Get()){
                    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
                    NRF_CLOCK->TASKS_HFCLKSTART = 1;
                    while(0 == NRF_CLOCK->EVENTS_HFCLKSTARTED);
                    ReceiverSC.state = SYSTEM_STATE_PREPARE_SCAN;
                }
            }break;
            case SYSTEM_STATE_SCANED:{
                if(0 == ReceiverSC.TipsPlayPointer){
                    Play_SetParam(ReceiverSC.Pair.Rate,ReceiverSC.Pair.AdcBits,ReceiverSC.Pair.FpwmDiv);
                    ReceiverSC.PlayVol = SOUND_VOLUME;
                    Play_Start();
                    ReceiverSC.state = SYSTEM_STATE_PREPARE_WORK;
                    ReceiverSC.NextTipsTime = SystemTick_Get() + BOND_TIPS_INTERVAL;                     
                }
            }break;
            case SYSTEM_STATE_PREPARE_WORK:{
                Rf_StartRx(ReceiverSC.RfBufPointer, ReceiverSC.RfBufLen, ReceiverSC.Pair.WorkChannel); 
                ReceiverSC.state = SYSTEM_STATE_WAIT;
                ReceiverSC.Timeout = SystemTick_Get();
            }break;
            case SYSTEM_STATE_WAIT:{
                if ( !User_WFI(8)){
                    if ( ReceiverSC.Timeout + 4000 < SystemTick_Get()) {
                        /* not recv, timeout */
                        ReceiverSC.state = SYSTEM_STATE_INITED;	
                        Log("TO:%d %d\n", ReceiverSC.Timeout, SystemTick_Get());								  
                    }else  if ( NRF_RADIO->STATE < 1 || NRF_RADIO->STATE > 3 ){
                        //Re-scan if RX is disabled.
                        Rf_StartRx(ReceiverSC.RfBufPointer, ReceiverSC.RfBufLen, ReceiverSC.Pair.WorkChannel); 
                    }
                }
            }break;
            case SYSTEM_STATE_RECV:{
                int Tmp, sleepMs, datalen = 0;
                uint32_t time;
                uint8_t *dev = (uint8_t *)&srcDevice;
                Rf_Stop();
                pChar = (unsigned char *)(ReceiverSC.RfBufPointer);
                PCMFrame *pInFrame = (PCMFrame *)pChar;

                dev[0] = pInFrame->dev[0];
                dev[1] = pInFrame->dev[1];
                dev[2] = pInFrame->dev[2];
                dev[3] = pInFrame->dev[3];

                time = SystemTick_Get();
                if ( NRF_RADIO->RSSISAMPLE > 85 ){
                    rssiLowCount++;
                }else{
                    rssiLowCount = 0;
                }
								
                //Time out OR Checksume is wrong and RSS is low OR other device data. 
                if (( ReceiverSC.Timeout + 24 < time ) 
                    || ( NRF_RADIO->CRCSTATUS == 0 && NRF_RADIO->RSSISAMPLE > 85 ) 
                    || ( ( RECV_MODE & RECV_MODE_BOND ) && ReceiverSC.boundedDeviceId != 0 && srcDevice != ReceiverSC.boundedDeviceId )
                    ){
                    Tmp = ReceiverSC.QuietFrameCnt;
                    if (Tmp > QUIET_SLEEP_MAX_DELAY) {
                        Tmp = QUIET_SLEEP_MAX_DELAY;
                    }
                    Tmp *= (ReceiverSC.Pair.Rate == SAMPLE_RATE_8000?16:8);
                    if (Tmp || (ReceiverSC.Timeout + 24 < time)){
                        NRF_CLOCK->TASKS_HFCLKSTOP = 1;
                    }
               #if ( WEBRTC_NS )
                    if ( ReceiverSC.Timeout + 24 < time ){                        
                        //When the signal is very weak, enter long time sleep.
                        sleepMs = 20;
                        Log("No data for long time.\n");
                    }else if( NRF_RADIO->CRCSTATUS == 0  && NRF_RADIO->RSSISAMPLE > 85 ){                         
                        //For data error.
                        if ( rssiLowCount > 10 ) sleepMs = 20;
                        else sleepMs = 5;
                        Log("CRC error %d.\n", rssiLowCount);
                    }else{                         
                        sleepMs = 10;
                        Log("Not bonded device data:%08x %08x.\n", srcDevice, ReceiverSC.boundedDeviceId);
                    }
                #else
                    if ( ReceiverSC.Timeout + 24 < time ){                        
                        //When the signal is very weak, enter long time sleep.
                        sleepMs = 32;
                        Log("No data for long time.\n");
                    }else if( NRF_RADIO->CRCSTATUS == 0  && NRF_RADIO->RSSISAMPLE > 85 ){                         
                        //For data error.
                        if ( rssiLowCount > 10 ) sleepMs = 32;
                        else sleepMs = 5;
                        Log("CRC error %d.\n", rssiLowCount);
                    }else{                         
                        sleepMs = 8;
                        Log("Not bonded device data:%08x %08x.\n", srcDevice, ReceiverSC.boundedDeviceId);
                    }
                #endif
                }else{
                    signed short *out = (signed short *)ReceiverSC.PlayBufPointer + (ReceiverSC.PlayBufRecvPos % ReceiverSC.PlayBufLen);                   
									
                    ReceiverSC.TotalFrames++;

                    datalen = rxFrameProcess( pChar, out );
                    
                    if ( ReceiverSC.QuietFrameCnt >= QUIET_START_FRAME_CNT && 
                         pInFrame->info.dtype.type == 0x00 ) {
                        Play_ShutDown();
                        NRF_CLOCK->TASKS_HFCLKSTOP = 1;
                        sleepMs = 0;
                        Tmp = ReceiverSC.QuietFrameCnt;
                        if (Tmp > QUIET_SLEEP_MAX_DELAY) {
                            Tmp = QUIET_SLEEP_MAX_DELAY;
                        }
                    #if ( WEBRTC_NS )
                        Tmp *= 10;
                    #else
                        Tmp *= (ReceiverSC.Pair.Rate == SAMPLE_RATE_8000?16:8);
                    #endif
                    }else{
                        if ( datalen > 0 ){
                            ReceiverSC.PlayBufRecvPos += datalen;
                            Play_Start();
                        }
                    
                        Tmp = 0;
                    /*
                        NOTE: A audio frame has 128((DATALEN_PER_FRAME - 4)*2) sample points,it costs 128/(sample rate) seconds.
                        For 8Khz,it is about 16ms(16*32.768=524 Ticks). 
                        Abandon the frame which repeat is 1 if the first frame has been received for power saving.
                     */  
                    #if ( WEBRTC_NS )
                        if ( pInFrame->info.dtype.repeat == 1) sleepMs = 4;
                        else sleepMs = 9;
                    #else
                        if ( ReceiverSC.Pair.Rate == SAMPLE_RATE_8000 ){                        
                            if ((pInFrame->id&0x01)==0x00){
                                if ( pInFrame->info.dtype.repeat == 1 || datalen == 0) sleepMs = 5;
                                else sleepMs = 11;
                            }else{
                                if ( pInFrame->info.dtype.repeat == 1 || datalen == 0) sleepMs = 15;
                                else sleepMs = 20;
                            }
                        }else{
                            //For SAMPLE_RATE_16000
                            if ((pInFrame->id&0x01)==0x00){
                                if ( pInFrame->info.dtype.repeat == 1) sleepMs = 3;
                                else sleepMs = 6;
                            }else{
                                if ( pInFrame->info.dtype.repeat == 1) sleepMs = 6;
                                else sleepMs = 9;
                            }
                        }
                    #endif                        
                    }                       
                }
	
                //Log("%03d %d %d %d\n", pInFrame->id, (int)pInFrame->info.dtype.repeat, time, Tmp + sleepMs);
                ReceiverSC.state = SYSTEM_STATE_PREPARE_WORK;                           								
                DataEventProcess( pInFrame, datalen?true:false );								
                SystemTick_ForceSleepMs(Tmp + sleepMs - 1);
                NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
                NRF_CLOCK->TASKS_HFCLKSTART = 1;
                while(0 == NRF_CLOCK->EVENTS_HFCLKSTARTED);
            }break;
            case SYSTEM_STATE_BOND:{						
                Rf_BondResponse(ReceiverSC.RfBufPointer, 
                                ReceiverSC.RfBufLen, 
                                ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]);
                Rf_StartRx( ReceiverSC.RfBufPointer, 
                            ReceiverSC.RfBufLen, 
                            ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]); 
                ReceiverSC.state = SYSTEM_STATE_BONDING;	
                ReceiverSC.Timeout = SystemTick_Get() + 16 + RND_Get()%16;
            }break;
            case SYSTEM_STATE_BONDING:{						
                if ( ReceiverSC.Timeout < SystemTick_Get()){
                    if ( ReceiverSC.Responses == 0){
                        ReceiverSC.state = SYSTEM_STATE_PREPARE_SCAN;													
                    }else{
                        ReceiverSC.Responses--;
                        ReceiverSC.state = SYSTEM_STATE_BOND;	
                    }
                    Log("Bonding: %d Chn:%d\n", ReceiverSC.Responses, ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]);
                }
            }break;
            case SYSTEM_STATE_BONDED:{
                if ( ReceiverSC.Timeout < SystemTick_Get()){
                    Rf_BondResponse( ReceiverSC.RfBufPointer, 
                                     ReceiverSC.RfBufLen, 
                                     ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]);
                    Rf_StartRx( ReceiverSC.RfBufPointer, 
                                ReceiverSC.RfBufLen, 
                                ReceiverSC.ScanChannel.ScanChannel[ReceiverSC.ScanChannel.CurChannel]); 
                    ReceiverSC.Timeout = SystemTick_Get() + 16 + RND_Get()%16;
                    if ( ReceiverSC.Responses > 0){
                        ReceiverSC.Responses--;
                    }else{								
                        if(ReceiverSC.TipsPlayPointer == 0 ){
                            if ( ReceiverSC.boundedDeviceId != 0 ){
                                Play_SetParam(SAMPLE_RATE_16000,ADC_BITS_14,FPWM_DIV_1);
                                ReceiverSC.TipsPlayPointer = (unsigned int)(Tips_Bonded);
                                ReceiverSC.TipsPlayLen = sizeof_Tips_Bonded;
                            }else{
                                Play_SetParam(SAMPLE_RATE_16000,ADC_BITS_14,FPWM_DIV_1);
                                ReceiverSC.TipsPlayPointer = (unsigned int)(Tips_Unbonded);
                                ReceiverSC.TipsPlayLen = sizeof_Tips_Unbonded;
                            }
                            ReceiverSC.state = SYSTEM_STATE_SCANED;
                            ReceiverSC.NextTipsTime = SystemTick_Get() + 5000;
                            ReceiverSC.PlayVol = BOND_VOLUME;
                            Play_Start();
                            Log("Bonded: %x\n", ReceiverSC.boundedDeviceId);
                        }
                    }
                }
            }break;
#if ( RECV_MODE	& RECV_MODE_BUTTON )				
            case SYSTEM_STATE_BUTTON_BEEP:{
                Rf_Stop();
                __WFE();
                Play_EventTips(Tips_ButtonPressed, sizeof_Tips_ButtonPressed, BOND_VOLUME);
            }
            break;
#endif						
#if ( RECV_MODE	& RECV_MODE_LOW_BATTERY )				
            case SYSTEM_STATE_LOWPOWER:{
                Rf_Stop();
                __WFE();
                Play_EventTips(Tips_LowPower, sizeof_Tips_LowPower, BOND_VOLUME);
            }
            break;
#endif						
            default:{
            }break;
        }
        
        if(ReceiverSC.TipsPlayPointer){
            short * p = (short *)(ReceiverSC.PlayBufPointer);
            short * pTips = (short *)(ReceiverSC.TipsPlayPointer);
            /* playing tips audio */
            if(ReceiverSC.PlayBufPlayPos + IMA_ADPCM_PCM_RAW_LEN/2 > ReceiverSC.PlayBufRecvPos){
                int i, Tmp;
                /* copy new data to play */
                Tmp = ReceiverSC.TipsPlayLen - ReceiverSC.PlayBufRecvPos;
                if(Tmp > IMA_ADPCM_PCM_RAW_LEN){
                    /* everytime we just copy IMA_ADPCM_PCM_RAW_LEN bytes */
                    Tmp = IMA_ADPCM_PCM_RAW_LEN;
                }else if(Tmp > 0){
                    /* copy remain data */
                }else{
                    Tmp = 0;
                    /* copy data is OVER,but maybe play not finish */
                    if(ReceiverSC.PlayBufPlayPos >= ReceiverSC.PlayBufRecvPos){
                        Play_ShutDown();
                    
                        if( ReceiverSC.state == SYSTEM_STATE_BUTTON_BEEP || 
                            ReceiverSC.state == SYSTEM_STATE_LOWPOWER ){
                            //Restore play setting.
                            ReceiverSC.state = SYSTEM_STATE_SCANED;
                        }
                        
                        //Log("Play tips down\n");
                    }
                }
                
                //Copy data
                p += ((ReceiverSC.PlayBufRecvPos) % (ReceiverSC.PlayBufLen));
                pTips += ReceiverSC.PlayBufRecvPos;
                for(i = 0;i < Tmp;i++){
                    p[i] = pTips[i];
                }
                ReceiverSC.PlayBufRecvPos += Tmp;
            }
        }
#ifdef TIPS_TEST        
        else{
            if (ReceiverSC.NextTipsTime<SystemTick_Get()){
                Play_SetParam(SAMPLE_RATE_16000,ADC_BITS_14,FPWM_DIV_1);
                ReceiverSC.TipsPlayPointer = (unsigned int)(Tips_Unconnect_Bond);
                ReceiverSC.TipsPlayLen = sizeof_Tips_Unconnect_Bond;               
                ReceiverSC.NextTipsTime = SystemTick_Get() + 5000;                     
                ReceiverSC.PlayVol = BOND_VOLUME;
                Play_Start();
            }
        } 
#endif
        
    }
    return 0;
}
