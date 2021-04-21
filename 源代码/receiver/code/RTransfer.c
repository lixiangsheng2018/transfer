/*
 * Transfer.c
 *
 * Copyright: Tomore
 *  Author: Lamy Xiang
 * Updated: 2016/6/15
 *
 */
#include <stdbool.h>
#include "Receiver.h"
#include "Queue.h"
#include "RTransfer.h"
#include "math.h"

//#define DEBUG_RECEIVER

#ifdef DEBUG_RECEIVER
typedef struct
{
    unsigned short st;
    unsigned short rt;
    unsigned short dif;
    unsigned char  id;
    unsigned char  type:4;
    unsigned char  repeat:4;
}FRAME_INF;

FRAME_INF  frameInf[256]={0};
unsigned char curFrame = 0;
int lostFrames = 0;
#endif

#define MAX_FRAME_STATES   32

typedef struct
{
    unsigned char  id;
    unsigned char  quiet;
    unsigned char  error;
    unsigned char  rssi;
}FRAME_STATE;

FRAME_STATE  frameStat[MAX_FRAME_STATES]={0};
unsigned char statFId   = 0;
unsigned char errFrames = 0;
unsigned char cntFrames = 0;

const int rawSize = IMA_ADPCM_PCM_RAW_LEN;

FrameInfo lastRxFramesId;
    
void rxTransferInit(void)
{    
    lastRxFramesId.id = 0;
    lastRxFramesId.info.dtype.repeat = 0x03;
    initQueue();
}

static bool isRepeatFrame(PCMFrame *pInFrame)
{       
    return ((lastRxFramesId.info.dtype.repeat != 0x03 ) && (pInFrame->id == lastRxFramesId.id));
}

static bool isDecodeFrame(PCMFrame *pInFrame)
{       
    return ( (pInFrame->info.dtype.repeat || frameStat[statFId].error==false)&& !isRepeatFrame(pInFrame));
}

/*
 * Statistics frame error ratio, lost package ratio, quiet frames and continue frames.
 */
static void rxFrameStatistic( PCMFrame *pInFrame )
{       
    bool error;
    uint8_t lastId = (MAX_FRAME_STATES + statFId-1)%MAX_FRAME_STATES;
    uint8_t *pChar = pInFrame->data;
    uint8_t Tmp, sum;
    uint16_t i;
     
    //Check sum and CRC state
    Tmp = pChar[3];
    pChar[3] = sum = 0;
    for (i = 0;i < DATALEN_PER_FRAME; i++) {
        sum += pChar[i];
    }
    pChar[3] = Tmp;
    error = ((Tmp % 0x80) != (sum % 0x80)?true:false);

    //Update error frames
    if ( frameStat[statFId].error != error ){
        if ( error == true && errFrames < MAX_FRAME_STATES ) errFrames++;
        if ( error == false && errFrames ) errFrames--;
    }

    frameStat[statFId].id = pInFrame->id;
    frameStat[statFId].quiet = (pInFrame->data[3] & 0x80)?1:0;
    frameStat[statFId].error = error;
    frameStat[statFId].rssi  = NRF_RADIO->RSSISAMPLE;

    //Update lost frames
    if (frameStat[statFId].id != (frameStat[lastId].id + 1)%256){
        uint8_t lostFs = (frameStat[statFId].id + 256 - frameStat[lastId].id - 1)%256;
        if( cntFrames > lostFs ) cntFrames-=lostFs;
        else cntFrames = 0;
        if (cntFrames == 0) errFrames = 0;
        ReceiverSC.ContinueFrames = 0;
    }else{
        if( cntFrames < MAX_FRAME_STATES ) cntFrames++;
        ReceiverSC.ContinueFrames++;
    }

    if ( frameStat[statFId].quiet && pInFrame->info.dtype.type == 0){
        ReceiverSC.QuietFrameCnt++;
    }else{
        ReceiverSC.QuietFrameCnt = 0;
    }
    
    //Log("FS:%d %d %d %d\n", statFId, frameStat[statFId].quiet, pInFrame->id, ReceiverSC.QuietFrameCnt);    
}

/*
 *rxFrameProcess
 * Input: frame -- Rx frame. 
 *Output: PCM data
 *  Note: Rx buffer->play buffer
 */
int rxFrameProcess(unsigned char* in, signed short * out)
{
    int ret = 0;
    PCMFrame *pInFrame = (PCMFrame *)in;

#ifdef DEBUG_RECEIVER
    frameInf[curFrame].st = (unsigned short)ReceiverSC.Timeout;
    frameInf[curFrame].rt = (unsigned short)SystemTick_Get();
    frameInf[curFrame].dif= ReceiverSC.PlayBufRecvPos - ReceiverSC.PlayBufPlayPos;
    frameInf[curFrame].id = pInFrame->id;
    frameInf[curFrame].type = pInFrame->type;
    frameInf[curFrame].repeat = pInFrame->repeat;

    if ((pInFrame->data[3]&0x80) == 0x00 && 
        lastRxFramesId.repeat != 0x0f && 
        lastRxFramesId.quiet == 0 && 
        !isRepeatFrame(pInFrame)){
        int lstF = (256 + pInFrame->id - lastRxFramesId.id - 1)%256;
        if (lstF){
            lostFrames += lstF;
            Log("Lost some %d frames before %d. total:%d T:%d %d.\n", lstF, pInFrame->id, lostFrames, frameInf[curFrame].rt, frameInf[(256 + curFrame-1)%256].rt);
        }
    }
    
    curFrame++;
#endif
   
    rxFrameStatistic(pInFrame);
      
    //Do it only when id is new.
    if ( isDecodeFrame(pInFrame) ){
        unsigned char *pChar = pInFrame->data; 

        IMA_ADPCM_BlockDecode( pChar, out, rawSize );
      
        //High error ratio and weak RSSI
        if ((errFrames > cntFrames/2 && cntFrames > MIN_CONTINUE_FRAMES) || 
            (frameStat[statFId].error == true && frameStat[statFId].rssi > 72) ||
            frameStat[statFId].quiet )
        {
            uint16_t i;
            for ( i = IMA_ADPCM_PCM_RAW_LEN; i>0; i--){
                out[i-1] = 0;
            }
            if (frameStat[statFId].rssi >72){
                Log("RSSI:%d.\n", NRF_RADIO->RSSISAMPLE);
            }else if (frameStat[statFId].quiet == 0){
                Log("High error ratio:%d/%d.\n", errFrames, cntFrames);
            }
        }

        lastRxFramesId.id = pInFrame->id;
        lastRxFramesId.info.dtype.type = pInFrame->info.dtype.type;
        lastRxFramesId.info.dtype.repeat = pInFrame->info.dtype.repeat;
        lastRxFramesId.quiet = frameStat[statFId].quiet;

        //Log("%03d %d\n", lastRxFramesId.id, frameStat[statFId].quiet);
        ret = rawSize;
    }
    
    statFId = (statFId + 1)%MAX_FRAME_STATES;    
    return ret;
}
