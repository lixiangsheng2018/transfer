#ifndef _RECEIVER_H_
#define _RECEIVER_H_
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nrf.h"

#define LD_RECEIVER             0
#define WEBRTC_NS               0

#define RECV_MODE_POWER         0x01
#define RECV_MODE_BOND          0x02
#define RECV_MODE_BUTTON        0x04
#define RECV_MODE_LOW_BATTERY   0x08
#if (LD_RECEIVER == 1)
#define RECV_MODE               (RECV_MODE_BOND)
#else
#define RECV_MODE               (RECV_MODE_BUTTON|RECV_MODE_LOW_BATTERY)
#endif

#define MAX_CHANNEL_NO          3

#define FREQ_LOW                1
#define FREQ_HIGH               0
#define FREQ_RANGE              FREQ_HIGH

#ifdef SERIAL_DEBUG
#define UART_TX_PIN             19
#define UART_RX_PIN             18
#define Log(...) printf(__VA_ARGS__)
extern void Uart_Init(void);
#else
#define Log(...)
#define Uart_Init(...)
#endif

#define BEEP_VOLUME             0.02
#define BOND_VOLUME             1.00

#if (WEBRTC_NS == 1)
#define DATALEN_PER_FRAME       44  
#define SOUND_VOLUME            1.00  
#elif (WEBRTC_NS == 2)
#define DATALEN_PER_FRAME       84  
#define SOUND_VOLUME            1.00  
#else
#define DATALEN_PER_FRAME       68  
#if (RECV_MODE & RECV_MODE_BUTTON)
#define SOUND_VOLUME            2.00  
#else
#define SOUND_VOLUME            1.50  
#endif
#endif

#define BOND_TIPS_INTERVAL      15000

#define IMA_ADPCM_4BIT_BLOCK    DATALEN_PER_FRAME
#define IMA_ADPCM_PCM_RAW_LEN   ((IMA_ADPCM_4BIT_BLOCK - 4) * 2)
#define IMA_US2S_BASE_VALUE     32768

//Definitions for quiet frame
#if (WEBRTC_NS)
#define QUIET_THREHOLD          0
#else
#define QUIET_THREHOLD          48
#endif
#define QUIET_START_FRAME_CNT   8
#define QUIET_SLEEP_MAX_DELAY   8
#define PLAY_AUDIO_DELAY_FRAMES 4
#define MIN_CONTINUE_FRAMES     16

#define PWM0_OUT_PIN            6
#define PWM1_OUT_PIN            8

typedef enum _SystemState {
    SYSTEM_STATE_INITED = 0,
    SYSTEM_STATE_PREPARE_SCAN,
    SYSTEM_STATE_SCAN,
    SYSTEM_STATE_SCANED,
    SYSTEM_STATE_SCANTIMEOUT,
    SYSTEM_STATE_PREPARE_WORK,
    SYSTEM_STATE_WAIT,
    SYSTEM_STATE_RECV,
    SYSTEM_STATE_BOND,
    SYSTEM_STATE_BONDING,
    SYSTEM_STATE_BONDED,
    SYSTEM_STATE_LOWPOWER,
    SYSTEM_STATE_BUTTON_BEEP,
    SYSTEM_STATE_EXIT,
}ReceiverSystemState;

typedef enum _SampleRate {
    SAMPLE_RATE_8000 = 0,
    SAMPLE_RATE_11025,
    SAMPLE_RATE_16000,
    SAMPLE_RATE_22050
}SampleRate;

typedef enum _AdcBits {
    ADC_BITS_8  = 0,
    ADC_BITS_10,
    ADC_BITS_12,
    ADC_BITS_14,
    ADC_BITS_16
}AdcBits;

typedef enum _FpwmDiv {
    FPWM_DIV_1 = 0,
    FPWM_DIV_2,
    FPWM_DIV_4,
    FPWM_DIV_8,
    FPWM_DIV_16
}FpwmDiv;

typedef enum _BroadcastType {
    BROADCAST_DEFAULT = 0,
    BROADCAST_BOND,
    BROADCAST_UNBOND,
    BROADCAST_CLEAR,
    BROADCAST_UNBOND_FORCE,
    BROADCAST_BOND_DEVICE,
    BROADCAST_UNBOND_DEVICE,
}BroadcastType;

typedef enum _EAUDIO_EVENT {
    AUDIO_EVENT_NONE = 0,
    AUDIO_EVENT_BUTTON,
    AUDIO_EVENT_LOWPOWER,
    AUDIO_EVENT_RECONNECT,
}EAudioEvent;

//Event Info
typedef struct
{
    EAudioEvent event;
    uint32_t    timeout;
}EventInfo;

/* struct define */
typedef struct _sys {
    ReceiverSystemState state;

    struct _scanchannel{
        unsigned char ScanChannel[3];
        unsigned char CurChannel;
    }ScanChannel;

    unsigned int Timeout;
    unsigned int PlayBufPointer;  //The buffer which the pointer pointed to is used circularly.
    unsigned int PlayBufLen;
    unsigned int PlayBufPlayPos;  //The pos will be always increased as playing.
    unsigned int PlayBufRecvPos;  //The pos will be always increased as received data.
    float PlayVol;
    
    unsigned int RfBufPointer;
    unsigned int RfBufLen;

    unsigned int TipsPlayPointer;
    unsigned int TipsPlayLen;
    unsigned int NextTipsTime;

    unsigned int QuietFrameCnt;
    unsigned int ContinueFrames;
    unsigned int TotalFrames;

    EventInfo eventInfo;

    struct _pair{
        unsigned char PairHeader[10];    /* 0xA55A + "DM" + 0x5AA5 */
        unsigned char WorkChannel;
        unsigned char Rate;
        unsigned char AdcBits;
        unsigned char FpwmDiv;
    }Pair;
    
    unsigned int deviceId;
    unsigned int boundedDeviceId;
    unsigned int Responses;
    
    unsigned int FlagRfRecv;
    unsigned int DataType;
}ReceiverSysCfg;

extern ReceiverSysCfg ReceiverSC;
extern const short Tips_Unconnect[];
extern const short Tips_Unconnect_Bond[];
extern const short Tips_Connect[];
extern const short Tips_Unbonded[];
extern const short Tips_Bonded[];
extern const short Tips_LowPower[];
extern const short Tips_ButtonPressed[];
extern const unsigned int sizeof_Tips_Unconnect;
extern const unsigned int sizeof_Tips_Unconnect_Bond;
extern const unsigned int sizeof_Tips_Connect;
extern const unsigned int sizeof_Tips_Unbonded;
extern const unsigned int sizeof_Tips_Bonded;
extern const unsigned int sizeof_Tips_LowPower;
extern const unsigned int sizeof_Tips_ButtonPressed;

void SystemTick_Init(void);
unsigned int SystemTick_GetTicks(void);
unsigned int SystemTick_Get(void);
void SystemTick_ForceSleepMs(unsigned int ms);
void SystemTick_ForceSleepTicks(unsigned int ticks);
bool User_WFI(unsigned int ms);

void Play_Init(void);
void Play_Start(void);
void Play_EventTips( const short tips[], uint32_t len, float volume );
void Play_Stop(void);
void Play_ShutDown(void);
void Play_SetParam(unsigned char rate,unsigned char adcbit,unsigned char fpwm_div);
unsigned short Play_GetState(void);

void Rf_Init(unsigned int TxAddr);
void Rf_StartRx(unsigned int bufPointer,unsigned int len,unsigned char channel);
void Rf_BondResponse(unsigned int bufPointer,unsigned int len,unsigned char channel);
void Rf_EnterScanMode(char scan);
void Rf_Stop(void);

void IMA_ADPCM_BlockDecode(unsigned char *indata,signed short *outdata, int len);
#endif

