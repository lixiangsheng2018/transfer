#include "sdk_errors.h"
#include "app_error.h"
#include "Sender.h"
#include "STransfer.h"
#include "Twis.h"
#include "Flash.h"
#include "Led.h"
#include "At.h"
#if (DENOISE_PROCESS == DENOISE_WEBRTC)
#include "NsLib.h"
#endif
SenderSysCfg SenderSC;

#define RF_TEST   0           

static signed short SampleBuffer[DATALEN_PER_RAWFRAME * MAX_SAMPLE_FRAMES];
static unsigned char RfSendRecvBuffer[256];

extern __IO bool sampleState;

void SendSC_Init(void){

    memset(&SenderSC, 0x00, sizeof(SenderSC));

#if (FREQ_RANGE == FREQ_HIGH)  
    //Frequency = 2400 + channel  
    SenderSC.ScanChannel.ScanChannel[0] = 88;
    SenderSC.ScanChannel.ScanChannel[1] = 82;
    SenderSC.ScanChannel.ScanChannel[2] = 94;
#else
    //Frequency = 2360 + channel  
    SenderSC.ScanChannel.ScanChannel[0] = 23;
    SenderSC.ScanChannel.ScanChannel[1] = 3;
    SenderSC.ScanChannel.ScanChannel[2] = 33;
#endif    
    SenderSC.rft.duration = (uint32_t)-1; 
    SenderSC.rft.channel = SenderSC.ScanChannel.ScanChannel[0];
  
    SenderSC.ScanChannel.CurChannel = 0;
    SenderSC.ScanChannel.NextBroadcastTime = NEXT_BOARDCAST_TIME_BASE_NUMBER;
    
#ifndef BUTTON_POWER_PIN
    SenderSC.state = SYSTEM_STATE_INITED;
    SenderSC.nRF52State = NRF52_ON;
    SenderSC.sim800State = SIM800_ON;
#endif
    
    SenderSC.Pair.UsedChannelBitMaps[(SenderSC.ScanChannel.ScanChannel[0])/32] |= 1 << ((SenderSC.ScanChannel.ScanChannel[0])%32);
    SenderSC.Pair.UsedChannelBitMaps[(SenderSC.ScanChannel.ScanChannel[1])/32] |= 1 << ((SenderSC.ScanChannel.ScanChannel[1])%32);
    SenderSC.Pair.UsedChannelBitMaps[(SenderSC.ScanChannel.ScanChannel[2])/32] |= 1 << ((SenderSC.ScanChannel.ScanChannel[2])%32);
    SenderSC.SampleBufPointer = (unsigned int)(SampleBuffer);
    SenderSC.SampleBufLen = sizeof(SampleBuffer)/sizeof(SampleBuffer[0]);
    SenderSC.SampleCurPos = 0;

    SenderSC.RfBufPointer = (unsigned int)RfSendRecvBuffer;
    SenderSC.RfBufLen = sizeof(RfSendRecvBuffer)/sizeof(RfSendRecvBuffer[0]);

    GetDevicesInfo(&SenderSC.deviceInfo);
    SenderSC.txAddress = (SenderSC.deviceInfo.addressId?*((unsigned int *)0x7FFF8):*((unsigned int *)0x7FFFC));
    
    SenderSC.Pair.PairHeader[0] = 0xA5;
    SenderSC.Pair.PairHeader[1] = 0x5A;
    SenderSC.Pair.PairHeader[2] = 'D';
    SenderSC.Pair.PairHeader[3] = 'M';
    SenderSC.Pair.PairHeader[4] = 0x5A;
    SenderSC.Pair.PairHeader[5] = 0xA5;
    memcpy(SenderSC.Pair.PairHeader + 6, &SenderSC.txAddress ,4);
        
    SenderSC.Pair.WorkChannel = SenderSC.ScanChannel.ScanChannel[0];
    SenderSC.Pair.AdcBits = ADC_BITS_14;
    SenderSC.Pair.FpwmDiv = FPWM_DIV_1;
    switch(SAMPLE_RATE)
    {
    case 22050: SenderSC.Pair.Rate = SAMPLE_RATE_22050; break;
    case 16000: SenderSC.Pair.Rate = SAMPLE_RATE_16000; break;
    case 11025: SenderSC.Pair.Rate = SAMPLE_RATE_11025; break;
    default: SenderSC.Pair.Rate = SAMPLE_RATE_8000; break;
    }
    
    //Initialize queue.
    ret_code_t err_code = initQueue();    
    APP_ERROR_CHECK(err_code);
  
    __enable_irq();

    Watchdog_Init(10);
    
    return;
}

void Broadcast_SetType( BroadcastType iType, EAudioEvent ev )
{
    if ( SenderSC.stateUpdated || SenderSC.state != SYSTEM_STATE_WORK ){
        //Debounce of BOND/UNBOND button
        return;        
    }
    
    SenderSC.brdcstType = iType;
    
    switch(SenderSC.brdcstType){
    case BROADCAST_BOND:
    case BROADCAST_UNBOND:
    case BROADCAST_CLEAR:    
    case BROADCAST_UNBOND_FORCE:
    case BROADCAST_BOND_DEVICE:
    case BROADCAST_UNBOND_DEVICE:
    {
        SenderSC.ScanChannel.Broadcasts = 80;
        SenderSC.newState = SYSTEM_STATE_BOND;
        SenderSC.stateUpdated = true;
        SenderSC.eventInfo.newEvent = ev;
    }break;
    case BROADCAST_ADDRESS_CHANGED:
    {
        SenderSC.ScanChannel.Broadcasts = 0;
        SenderSC.newState = SYSTEM_STATE_ADDRESS_CHANGED;
        SenderSC.stateUpdated = true;
        SenderSC.eventInfo.newEvent = ev;
    }break;
    default: break;    
    }
}

/*Process bond event.
 *Parameter: QuietPackages - the number of continue quiet frame.
 *Output: SenderSC.eventInfo.oldEvent
 *Note: 
 *    1. It maybe change the sender state to SYSTEM_STATE_BROADCAST when broadcast time event arrived.
 *       or SYSTEM_STATE_BOND when new bond command is received.
 *    2. New event will overwrite old event.
 */
EAudioEvent EventProcess(int QuietPackages )
{
    if( SenderSC.eventInfo.oldEvent != AUDIO_EVENT_NONE ){
        if ( SenderSC.eventInfo.onCount > 0 ){
            SenderSC.eventInfo.onCount--;
        }else{                                  
            SenderSC.eventInfo.oldEvent = AUDIO_EVENT_NONE;
        }
        SenderSC.eventInfo.dnCount = 0;
    }else if( SenderSC.eventInfo.newEvent != AUDIO_EVENT_NONE ) {
        if( SenderSC.eventInfo.dnCount >=2 || SenderSC.eventInfo.updated ){
            SenderSC.eventInfo.dnCount = 0;
            SenderSC.eventInfo.oldEvent = SenderSC.eventInfo.newEvent;
            SenderSC.eventInfo.updated = false;
            SenderSC.eventInfo.onCount = (QuietPackages >= QUIET_START_FRAME_CNT?6:3);
            SenderSC.eventInfo.newEvent = AUDIO_EVENT_NONE;                        
        }else{
            SenderSC.eventInfo.dnCount++;
        }
    }else{
        SenderSC.eventInfo.dnCount++;
    }
	
    SenderSC.eventInfo.evCount++;
    if((SenderSC.eventInfo.evCount%NEXT_BOARDCAST_TIME_BASE_NUMBER) == 0){
        SenderSC.ScanChannel.NextBroadcastTime = RND_Get()%NEXT_BOARDCAST_TIME_BASE_NUMBER;
    }			
		
    if ((SenderSC.eventInfo.evCount%NEXT_BOARDCAST_TIME_BASE_NUMBER) == SenderSC.ScanChannel.NextBroadcastTime) {
        SenderSC.state = SYSTEM_STATE_BROADCAST;
    }else if ( SenderSC.stateUpdated && 
               SenderSC.eventInfo.newEvent == AUDIO_EVENT_NONE && 
               SenderSC.eventInfo.oldEvent == AUDIO_EVENT_NONE ){
         SenderSC.stateUpdated = false;
         SenderSC.state = SenderSC.newState;
    }    
         
    return SenderSC.eventInfo.oldEvent;    
}

#if (VAD_ENABLE == 1)
//VAD - Voice active detect.
#define LOW_THRESHOLD     (QUIET_THREHOLD-16)
#define HIGH_THRESHOLD    (QUIET_THREHOLD+16)
uint8_t quietThreshold = QUIET_THREHOLD;
static int Diff[MAX_STAT_FRAMES]={0};
static int Zcc[MAX_STAT_FRAMES]={0};
static uint8_t frameState[MAX_STAT_FRAMES]={0};
static uint8_t frameId = 0;

uint8_t UpdateFrameState(signed short * SrcSignalArray){
    const int len = DATALEN_PER_RAWFRAME;
    int i;
    int Total,Average;
    int diff;	//Mean difference sum.   
    int zcc;  //Zero-crossing rate 

    Total = diff = zcc = 0;
    for (i = 0;i < len;i++) {
        Total += SrcSignalArray[i];
        if ( i > 0 && (SrcSignalArray[i]&0x8000) != (SrcSignalArray[i-1] & 0x8000)){
            zcc++;					
        }
    }
    Average = Total/len;
    for (i = 0;i < len;i++) {
        diff += abs((int)(SrcSignalArray[i] - Average));
    }
    diff /= len;
				
    Diff[frameId] = diff;
    Zcc[frameId]  = zcc;
    if (diff > quietThreshold ){
        frameState[frameId] = 2;
    }else{
        frameState[frameId] = 0;
    }

    uint8_t soundframes = 0;
    for ( i = 1; i<MAX_STAT_FRAMES; i++){
        if ( frameState[(frameId - i)%MAX_STAT_FRAMES] == 2 ){
            soundframes++;
        }
    }		
    
    //Adjust quiet threshold if possible.		
    if ( soundframes >= MAX_STAT_FRAMES/2 && frameState[frameId] == 2 ){
        if( quietThreshold+8 <= HIGH_THRESHOLD && quietThreshold < diff-8 ){					
            quietThreshold+=8;
        }
    }else if( soundframes == 0 && frameState[frameId] == 0 ){
        if( quietThreshold-8 >= LOW_THRESHOLD && quietThreshold > diff+8 ){
            quietThreshold-=8;
        }
    }		
    
    Log("VAD[%d]:%d %d %d %d\n", (int)frameId,(uint16_t)frameState[frameId], (uint16_t)quietThreshold, diff, zcc);
    frameId = (frameId + 1)%MAX_STAT_FRAMES;
    
    return frameId;
}

FTYPE GetFrameState( uint32_t fid ){
    uint8_t state = frameState[fid];
  
    if ( state == 0){
        uint8_t i, soundframes = 0;
        for ( i = 1; i<FORWARD_FRAMES; i++){
            if ( frameState[(fid + i)%MAX_STAT_FRAMES] == 2 ){
                soundframes++;
                break;
            }
        }		
    
        //Estimate smoothness frame.
        if ( soundframes || (Diff[fid] > quietThreshold/2 && Zcc[fid]>=30 && Zcc[fid]<=70) ){
            state = 1;
            Log("Forward:%d\n", (int)i);
        }else{
            soundframes = 0;
            for ( i = 1; i<BACKWARD_FRAMES; i++){
                if ( frameState[(fid - i)%MAX_STAT_FRAMES] == 2 ){
                    soundframes++;
                    break;
                }
            }		      
            
            if ( soundframes ){
                state = 1;            
                Log("Backward:%d\n", (int)i);
            }
        }
    }    
		
    Log("FRM[%d]:%d %d %d\n", (int)fid, (int)state, (int)frameId, fid);
    return state==0?FT_QUIET:FT_DATA;
}
#else
 bool IsQuietFrame(signed short * SrcSignalArray ){
    const int subFrames = 4*8000/SAMPLE_RATE;
    int len = DATALEN_PER_RAWFRAME / subFrames;
    int Total,Average,Diff,i,j;
	
    bool bQuiet = false;
	
    for (j=0; j<subFrames; j++){
        Total = Diff = 0;
        for (i = 0;i < len;i++) {
            Total += SrcSignalArray[i];
        }
        Average = Total/len;
        for (i = 0;i < len;i++) {
            Diff += abs((int)(SrcSignalArray[i] - Average));
        }
        Diff /= len;
				
        //If has one quiet sub-frame, big frame will be thought of quiet frame.				
        if (Diff < QUIET_THREHOLD && bQuiet == false ) bQuiet = true;
    }
    
    
    return ftype;
}
#endif

void PostMessage(Message id)
{
    uint8_t msgId = id;
    Frame msgFrame;
    msgFrame.data = &msgId;
    msgFrame.size = 1;
    pushQueue( &msgQ, &msgFrame );        
}

Message GetMessage(void)
{
    uint8_t msgId = MSG_IDLE;
    
    //Get message from other interface, such as IIC, UART.
    Frame msgFrame;
    msgFrame.data = &msgId;
    msgFrame.size = 1;
    pullQueue( &msgQ, &msgFrame );

    if ( msgId == MSG_IDLE ){
        if( BatteryLevelDetect() == BATT_GRADE_LOW ){
            if ( SenderSC.nRF52State == NRF52_ON ){
                msgId = MSG_LOW_BATTERY;
            }
        }else if ( SenderSC.nRF52State == NRF52_ON ){
            if(AudioLineIsChanged()){
                msgId = MSG_AUDIO_LINE_CHANGED;
            }
#ifdef AT_FUNCTIONS            
            else if(SimCardDetect()){
                msgId = MSG_SIMCARD_DETECT;
            }        
#endif            
        }        
    }
    
    return (Message)msgId;    
}

void MessageUpdate(void)
{
    static SenderSystemState oldState = SYSTEM_STATE_POWER_OFF;
      
    Watchdog_Feed();

    if (SenderSC.FlagRfRecv){
        SenderSC.FlagRfRecv = 0;
        Rf_ReceiveUpdate();
    }else{
        Message msgId = GetMessage();
        MessageProcess(msgId);
        
        if ( oldState != SenderSC.state){
            if( oldState == SYSTEM_STATE_POWER_OFF ){
            #if RF_TEST              
                Rf_TestStart( 88, false, (uint32_t)-1);
            #endif  
                Log("Power on - HW:%d-%d-%d. SND:%d-%d/%d\n",HARD_BOARD,UNBOUND_FORCE,AUTO_POWERON_ON_CHARGING, SAMPLE_RATE, SenderSC.deviceInfo.volume, VOLUME_1_0);
            }else if (SenderSC.state == SYSTEM_STATE_POWER_OFF ){
                Log("Power off.\n");
            }else{
                //Log("S:%d\n", SenderSC.state );
            }
            oldState = SenderSC.state;
        }
    }
    
    TWIS_McuStateUpdate();
}

void WebRtcNs_Callback( void )
{
    FrameProcess();  
}

int main(void){     
    //Init system
    SystemClock_Init();
    SendSC_Init();
    SystemTick_Init();
    Rf_Init(); 
    Uart_Init(UART_RX_PIN, UART_TX_PIN);
    Sample_Init();
    Led_Init();
    TWIS_Init();
    BSP_GpioInit();    
    Noise_Init();                

    Log("Sender Build @ %s %x %08x %d %d\r\n", __TIME__, NRF_FICR->DEVICEID[0], SenderSC.txAddress, SenderSC.deviceInfo.volume, SenderSC.Pair.Rate);
    
    while (SYSTEM_STATE_EXIT != SenderSC.state){
        //Process RADIO, BUTTON, MIC event.
        MessageUpdate();
        
        switch (SenderSC.state) {
            case SYSTEM_STATE_INITED:{
							  /*Scan broadcast channel to get used work channel information in order to avoid using those channels.*/
                Rf_StartRx(SenderSC.RfBufPointer, SenderSC.RfBufLen, SenderSC.ScanChannel.ScanChannel[SenderSC.ScanChannel.CurChannel]);
                SenderSC.ScanChannel.ScanTimeout = SystemTick_Get() + 1000;
                SenderSC.state = SYSTEM_STATE_SCAN;
            }break;
            case SYSTEM_STATE_SCAN:{
                if (SenderSC.ScanChannel.ScanTimeout < SystemTick_Get()) {
                    /* scan timeout, change to next channel */
                    SenderSC.ScanChannel.CurChannel = (SenderSC.ScanChannel.CurChannel+1)%MAX_CHANNEL_NO;

 		                if (SenderSC.ScanChannel.CurChannel == 0 ) {
											  //Scan is done.
                        SenderSC.state = SYSTEM_STATE_SCANED;
                    }else{
                        SenderSC.state = SYSTEM_STATE_INITED;
                    }
                }
            }break;
            case SYSTEM_STATE_SCANED:{
                /* we can get all USED channel when run into here */
            #if (FREQ_RANGE == FREQ_HIGH)                        
                int tmp = 95 - (RND_Get() % 15);
            #else
                int tmp = (RND_Get() % 40);
            #endif
                /* find rand unused channel */
                if (0 == (SenderSC.Pair.UsedChannelBitMaps[tmp/32] & (1 << (tmp%32)))) {
                    SenderSC.Pair.WorkChannel = tmp;
                    SenderSC.state = SYSTEM_STATE_PREPARE_WORK;
                }
            }break;
            case SYSTEM_STATE_PREPARE_WORK:{
                Log("Sample start\n");
                Sample_Start();
                while (SenderSC.SampleCurPos < AUDIO_CACHE_LENGTH); // cache audio              
                SenderSC.state = SYSTEM_STATE_WORK; 
            }break;
            case SYSTEM_STATE_WORK:{
                static int continueQuietPackage = 0;
                
                /* wait for sample over ,then send it */
                if ( SenderSC.SampleCurPos - SenderSC.DenoisePos >= 2*DATALEN_PER_RAWFRAME ) {
                    /* Denoise,everytime it just shift IMA_ADPCM_PCM_RAW_LEN/2 */
                    SignalProcess_Run((signed short *)SenderSC.SampleBufPointer,SenderSC.SampleBufLen,&(SenderSC.DenoisePos));
                    if (0 == (SenderSC.DenoisePos % DATALEN_PER_RAWFRAME)) {   //A frame OK
                        short * pTmp = (short *)SenderSC.SampleBufPointer; 
                        pTmp += ((SenderSC.DenoisePos - DATALEN_PER_RAWFRAME) % SenderSC.SampleBufLen);
                        uint8_t dfId = UpdateFrameState(pTmp);
                        if ( SenderSC.SampleSendPos + AUDIO_CACHE_LENGTH <= SenderSC.DenoisePos ){
                            uint8_t sfid = (MAX_STAT_FRAMES + dfId - AUDIO_CACHE_LENGTH/DATALEN_PER_RAWFRAME)%MAX_STAT_FRAMES;
                            FTYPE fType = GetFrameState( sfid );
                            if((fType & FT_DATA) != FT_DATA ){
                                /* Quiet frame */
                                SenderSC.SampleFrameQuietCnt++;
                                continueQuietPackage++;
                            }else{
                                continueQuietPackage = 0;
                            }
                            EAudioEvent ev = EventProcess( continueQuietPackage );
                        
                            //Log("S0[%02d]=%d S1[%02d]=%d %d\n", fId0, rawFrameState[fId0], fId1, rawFrameState[fId1], fType);
                            pTmp = (short *)SenderSC.SampleBufPointer; 
                            pTmp += (SenderSC.SampleSendPos % SenderSC.SampleBufLen);
                            rawDataProcess( pTmp, DATALEN_PER_RAWFRAME, fType, ev );
                            SenderSC.NextSendTime = SenderSC.SampleCurPos;
                            SenderSC.SampleSendPos += DATALEN_PER_RAWFRAME;
                        }
                    }
                }
            #if ( TRANSFER_MODE == TRANSFER_AFTER_PROCESSED )
                if( SenderSC.SampleCurPos >= SenderSC.NextSendTime ){
                    FrameProcess();
                    SenderSC.NextSendTime = SenderSC.SampleCurPos + TRANSFER_BASE - 24;    
                }
            #endif          
            }break;
            case SYSTEM_STATE_BROADCAST:{
                Rf_Broadcast( SenderSC.RfBufPointer, 
                              SenderSC.RfBufLen, 
                              SenderSC.ScanChannel.ScanChannel[SenderSC.ScanChannel.CurChannel],
                              0, 0 );
                //Log("Broadcast:%d\n", SenderSC.ScanChannel.ScanChannel[SenderSC.ScanChannel.CurChannel]);
                SenderSC.ScanChannel.CurChannel = (SenderSC.ScanChannel.CurChannel+1)%MAX_CHANNEL_NO;
                SenderSC.ScanChannel.NextBroadcastTime = NEXT_BOARDCAST_TIME_BASE_NUMBER;
                SenderSC.state = SYSTEM_STATE_WORK;
            }break;
            case SYSTEM_STATE_BOND:{
                Sample_Stop();
                Rf_Broadcast( SenderSC.RfBufPointer, 
                              SenderSC.RfBufLen, 
                              SenderSC.ScanChannel.ScanChannel[SenderSC.ScanChannel.CurChannel],
                              SenderSC.brdcstType,
                              SenderSC.connectedDeviceId );
                Rf_StartRx( SenderSC.RfBufPointer, 
                            SenderSC.RfBufLen, 
                            SenderSC.ScanChannel.ScanChannel[SenderSC.ScanChannel.CurChannel]);
                SenderSC.ScanChannel.ScanTimeout = SystemTick_Get() + 85 + RND_Get()%16;
                SenderSC.state = SYSTEM_STATE_BONDING;
                Log("Bonding..%d %d %d.\n", SenderSC.ScanChannel.Broadcasts, 
                                            SenderSC.brdcstType, 
                                            SenderSC.ScanChannel.ScanChannel[SenderSC.ScanChannel.CurChannel] );                
            }break;
            case SYSTEM_STATE_BONDING:{
                if (SenderSC.ScanChannel.ScanTimeout < SystemTick_Get()) {
                    //Scan broadcast channel before connected with a receiver.
                    if ( SenderSC.brdcstType == BROADCAST_BOND 
                      || SenderSC.brdcstType == BROADCAST_UNBOND 
                      || SenderSC.brdcstType == BROADCAST_UNBOND_FORCE ){
                        //Select broadcast channel.
                        SenderSC.ScanChannel.CurChannel = (SenderSC.ScanChannel.CurChannel+1)%MAX_CHANNEL_NO;
                    }
                    
                    if ( SenderSC.ScanChannel.Broadcasts > 0){
                        SenderSC.state = SYSTEM_STATE_BOND;
                        SenderSC.ScanChannel.Broadcasts--;
                    }else{
                        SenderSC.state = SYSTEM_STATE_PREPARE_WORK;
                        SenderSC.brdcstType = BROADCAST_DEFAULT;
                    }
                }
            }break;
            case SYSTEM_STATE_BONDED:{
                int i;
                if ( SenderSC.brdcstType == BROADCAST_BOND_DEVICE ){
                    for( i = 0; i<MAX_BONDED_DEVICES; i++){
                        if (SenderSC.deviceInfo.bondedDevices[i] == 0 ){
                            //Find a free room.
                            SenderSC.deviceInfo.bondedNum = i + 1;
                            SenderSC.deviceInfo.bondedDevices[i] = SenderSC.connectedDeviceId;
                            UpdateDevicesInfo();
                            TWIS_DevicesUpdate();
                            Log("Bonded:%x %d\n", SenderSC.connectedDeviceId, SenderSC.deviceInfo.bondedNum);
                            break;
                        }else if( SenderSC.deviceInfo.bondedDevices[i] == SenderSC.connectedDeviceId){
                            break;
                        }
                    }
                }else if ( SenderSC.brdcstType == BROADCAST_UNBOND_DEVICE ||
                           SenderSC.brdcstType == BROADCAST_UNBOND_FORCE ){
                    for( i = 0; i<MAX_BONDED_DEVICES; i++){
                        if (SenderSC.deviceInfo.bondedDevices[i] == 0 ){
                            break;
                        }else if( SenderSC.deviceInfo.bondedDevices[i] == SenderSC.connectedDeviceId){
                            int j;
                            for(j=i; j<MAX_BONDED_DEVICES-1; j++){ 
                                SenderSC.deviceInfo.bondedDevices[j] = SenderSC.deviceInfo.bondedDevices[j+1];
                                if ( SenderSC.deviceInfo.bondedDevices[j] == 0 ){
                                    break;
                                }
                            }
                            SenderSC.deviceInfo.bondedNum = j;
                            UpdateDevicesInfo();
                            TWIS_DevicesUpdate();
                            Log("Unbonded:%x %d\n", SenderSC.connectedDeviceId, SenderSC.deviceInfo.bondedNum);
                            break;
                        }
                    }
                }
                Log("Bond/Unbond finish: %x %d\n", SenderSC.connectedDeviceId, SenderSC.deviceInfo.bondedNum);
                SenderSC.state = SYSTEM_STATE_PREPARE_WORK;
                SenderSC.brdcstType = BROADCAST_DEFAULT;
            }break;
            case SYSTEM_STATE_ADDRESS_CHANGED:{
                Rf_AddressUpdate();                
                SenderSC.stateUpdated = false;
                SenderSC.state = SYSTEM_STATE_PREPARE_WORK;
                SenderSC.brdcstType = BROADCAST_DEFAULT;
                Log("RF Address:%08x\n", SenderSC.txAddress);
            }break;
            case SYSTEM_STATE_RF_TEST:{
                if ( Rf_Test() == false ){
                    //Power on if exit RF_Test mode.
                    SenderSC.state = (SenderSC.rft.state?SYSTEM_STATE_INITED:SYSTEM_STATE_POWER_OFF);
                }                    
            }break;
            default:{
            }break;
        }
    }
    return 0;
}

