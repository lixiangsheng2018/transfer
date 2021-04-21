/*
 * STransfer.c
 *
 * Copyright: Tomore
 *  Author: Lamy Xiang
 * Updated: 2016/6/15
 */
#include <stdbool.h>
#include "sdk_errors.h"
#include "app_error.h"
#include "Sender.h"
#include "Queue.h"
#include "STransfer.h"
    
extern SenderSysCfg SenderSC;
extern void IMA_ADPCM_BlockDecode(unsigned char *indata,signed short *outdata, int len);

static bool framesUpdated = false;
static unsigned char frameId = 0;
static PCMFrame lastPCMFrame;
static Frame lastTxFrames;

void RTC2_IRQHandler(void){

    if (NRF_RTC2->EVENTS_COMPARE[0]) {
        FrameProcess();
      
        NRF_RTC2->EVENTS_COMPARE[0] = 0;
        (void)(NRF_RTC2->EVENTS_COMPARE[0]);
        NRF_RTC2->TASKS_CLEAR = 1;
    }
}

void TransferReset(void)
{
    SenderSC.DenoisePos = 0;
    SenderSC.SampleCurPos = 0;
    SenderSC.SampleSendPos = 0;
    
    //Initialize variables.
    framesUpdated = false;
    frameId = 0;
    lastTxFrames.size = 0x00;
    lastTxFrames.data = (uint8_t *)&lastPCMFrame;
    
#if (TRANSFER_MODE == TRANSFER_AUTO)
    NRF_RTC2->TASKS_STOP = 1;
    NRF_RTC2->TASKS_CLEAR = 1;
    NRF_RTC2->PRESCALER = 0;
    NRF_RTC2->CC[0] = TRANSFER_BASE*1000/SAMPLE_RATE*32;  //Included a broadcast frame transfer time per 60frames
    NRF_RTC2->INTENSET = (1 << 16);//enable CC0
    NRF_RTC2->TASKS_START = 1;
    NVIC_EnableIRQ(RTC2_IRQn);
#endif  
}

/*****************************************
 * void FrameProcess( void )
 */
void FrameProcess( void )
{
    if ( framesUpdated == false ){
        //Do nothing if data isn't ready.
        if ( isQueueNotEmpty(&audioQ) == DATA_INVALID ){
            return ;
        }
        
        //Get new frame
        framesUpdated = true;
        pullQueue(&audioQ, &lastTxFrames);
    }
    
    PCMFrame *pFrame = &lastPCMFrame;
    
    if (pFrame->id == 0 && (pFrame->info.value & 0x80) ){
        //For broadcast
        Rf_SetMode(RND_Get()%2);

        NRF_RADIO->TXADDRESS = 0x00;
        Rf_SendPackage( (unsigned int)pFrame->data, lastTxFrames.size-2, pFrame->info.value & 0x7F);
        NRF_RADIO->TXADDRESS = 0x01;    
        
        framesUpdated = false;      
    }else{
        static int mode = 0;
      
        if ( (pFrame->info.dtype.repeat%2) == 0 ){
            mode = RND_Get()%2;
        }else{
            mode = 1 - mode;
        }
        
        //For ADPCM data.
        Rf_SetMode(mode);
    
        Rf_SendPackage((unsigned int)pFrame, lastTxFrames.size, SenderSC.Pair.WorkChannel); 
        
        if( pFrame->info.dtype.repeat == MAX_REPEAT ){
            framesUpdated = false;
        }
        pFrame->info.dtype.repeat = (pFrame->info.dtype.repeat+1)%(MAX_REPEAT+1);
    }
}

void broadcastProcess( char *info, int len, unsigned char channel )
{
    const char *dev = (char*)&SenderSC.deviceInfo.deviceId;
    int i;
    PCMFrame pcmFrame;
    Frame broadFrame;
    broadFrame.data = (uint8_t *)&pcmFrame; 
    broadFrame.size = len + 6;
    
    pcmFrame.dev[0] = dev[0];
    pcmFrame.dev[1] = dev[1];
    pcmFrame.dev[2] = dev[2];
    pcmFrame.dev[3] = dev[3];
    pcmFrame.id     = 0;
    pcmFrame.info.value   = 0x08 | channel;
  
    for (i=0; i<len;i++){
        pcmFrame.data[i] = info[i];      
    }

    if (isQueueFull(&audioQ) == DATA_VALID){        
        Log("Broadcast Queue is busy.\n");
    }
        
    //Push frame to transfer queue.
    pushQueue(&audioQ, (Frame*)&broadFrame);
}

/*
 *rawDataProcess
 * Input: source -- raw data. 
 *Output: IMA_ADPCM frame data.
 *  Note: Raw data -> transmit queue.
 */
void rawDataProcess( short *source, int len, FTYPE fType, EAudioEvent ev )
{
    const int Subpackets = DATALEN_PER_RAWFRAME/IMA_ADPCM_PCM_RAW_LEN;
    const int dataLen = len/Subpackets;
    const char *dev = (char*)&SenderSC.deviceInfo.deviceId;
    int j;
    PCMFrame pcmFrame;
    Frame frame;  
    frame.data = (uint8_t*)&pcmFrame;

    for( j=0; j<len;j++){
        source[j] = source[j]*1.0f*SenderSC.deviceInfo.volume/VOLUME_1_0;  //Adjust volume.  
#if WHITE_NOISE_LEVEL
        //Add white noise.
        source[j] += (rand()%(WHITE_NOISE_LEVEL<<4)) - (WHITE_NOISE_LEVEL<<3);
#endif
    }
  
    j = Subpackets;
    while(j){
        unsigned char *pChar = pcmFrame.data;
        short *pTmp = source+((Subpackets-j)*dataLen);
        int sum = 0;
        
        int pcmlen = IMA_ADPCM_BlockEncode(pTmp, pChar,dataLen);
        /* Calc sum */
        pChar[3] = 0;

        for(int i = 0; i<pcmlen; i++) {
           sum += pChar[i];
        }
        pChar[3] = (sum % 0x80);  //set sum
        if( (fType & FT_DATA) != FT_DATA ) {
           pChar[3] |= 0x80;
        }          

        if (isQueueFull(&audioQ) == DATA_VALID){        
            Log("Queue is busy.\n");
        }
        
        //Log("%03d %d %d\n", frameId, fType, pChar[3]&0x80?1:0);
        
        //Push frame to transfer queue.
        frame.size = pcmlen+6; 
        pcmFrame.dev[0] = dev[0];
        pcmFrame.dev[1] = dev[1];
        pcmFrame.dev[2] = dev[2];
        pcmFrame.dev[3] = dev[3];
        pcmFrame.id     = frameId++;
        pcmFrame.info.dtype.evt  = ev;
        pcmFrame.info.dtype.mode = SenderSC.deviceInfo.datatype||SenderSC.deviceInfo.bondedNum==0?0x01:0x00;
        pcmFrame.info.dtype.type = (fType&FT_LOADING?1:0);
        pcmFrame.info.dtype.repeat = 0;
        pushQueue(&audioQ, (Frame*)&frame);
        
        j--;
    }
}
