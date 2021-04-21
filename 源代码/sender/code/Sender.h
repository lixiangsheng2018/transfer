#ifndef _SENDER_H_
#define _SENDER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdint.h"
#include "stdbool.h"
#include "nrf.h"
#include "Defines.h"

#ifdef SERIAL_DEBUG
#define Log(...) printf(__VA_ARGS__)
#else
#define Log(...)
#endif
extern void Uart_Init( uint8_t rxPin, uint8_t txPin );

typedef enum _SystemState {
    SYSTEM_STATE_POWER_OFF = 0,
    SYSTEM_STATE_INITED,
    SYSTEM_STATE_SCAN,
    SYSTEM_STATE_SCANED,
    SYSTEM_STATE_PREPARE_WORK,
    SYSTEM_STATE_WORK,
    SYSTEM_STATE_BROADCAST,
    SYSTEM_STATE_BOND,
    SYSTEM_STATE_BONDING,
    SYSTEM_STATE_BONDED,
    SYSTEM_STATE_ADDRESS_CHANGED,
    SYSTEM_STATE_RF_TEST,
    SYSTEM_STATE_EXIT,
}SenderSystemState;

typedef enum _RF_TYPE {
    RFT_NONE = 0x00,
    RFT_RX   = 0x01,
    RFT_TX   = 0x02,
}RF_TYPE;

typedef enum _SAMPLE_SRC {
    SSRC_NOISE = 0x00,
    SSRC_MIC,
    SSRC_DATA,
}SAMPLE_SRC;

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

typedef enum _SIM800_STATUS {
    SIM800_OFF = 0,
    SIM800_ON,
    SIM800_POWER_GOON,
}Sim800PowerState;

typedef enum _BATT_GRADE {
    BATT_GRADE_LOW = 0,
    BATT_GRADE_MID_LOW,
    BATT_GRADE_WARNING,
    BATT_GRADE_1,
    BATT_GRADE_2,
    BATT_GRADE_3,
    BATT_GRADE_UNKNOWN,
}BattGrade;

typedef enum _NRF52_STATUS {
    NRF52_OFF = 0,
    NRF52_ON,
}nRF52PowerState;

typedef enum _FRAME_TYPE {
    FT_QUIET   = 0x00,
    FT_DATA    = 0x01,
    FT_LOADING = 0x02,
}FTYPE;

typedef enum _BroadcastType {
    BROADCAST_DEFAULT = 0,
    BROADCAST_BOND,
    BROADCAST_UNBOND,
    BROADCAST_CLEAR,	
    BROADCAST_UNBOND_FORCE,
    BROADCAST_BOND_DEVICE,
    BROADCAST_UNBOND_DEVICE,
    BROADCAST_ADDRESS_CHANGED,
}BroadcastType;

/* struct define */
typedef struct _deviceinfo {
    uint32_t size;
    uint32_t bondedNum;
    uint32_t bondedDevices[MAX_BONDED_DEVICES];
    uint32_t volume;
    uint32_t datatype;
    uint32_t addressId;
    uint32_t deviceId;
    uint32_t reserved[2];
}DevicesInfo;

typedef enum _EAUDIO_EVENT {
    AUDIO_EVENT_NONE = 0,
    AUDIO_EVENT_BUTTON,
    AUDIO_EVENT_LOWPOWER,
    AUDIO_EVENT_RECONNECT,
}EAudioEvent;

typedef struct _Event_Info{
    EAudioEvent newEvent;
    EAudioEvent oldEvent;
    bool        updated;
    uint8_t     onCount;
    uint8_t     dnCount;
    uint8_t     evCount;
}EventInfo;

typedef enum _MESSAGE{
    MSG_IDLE,
    MSG_LOW_BATTERY,
    MSG_POWER_KEY,
    MSG_UART_RX,
    MSG_AUDIO_LINE_CHANGED,
    MSG_SIMCARD_DETECT,
}Message;

typedef struct _RF_TEST{
    uint8_t channel;
    bool dir;
    uint32_t duration;
    SenderSystemState state;
}Rftest;

/* struct define */
typedef struct _sys {
    __IO SenderSystemState state;
    SenderSystemState newState;
    __IO bool stateUpdated;
	
	  bool micIn;
	  bool csmCardIn;
	  Sim800PowerState sim800State;
	  nRF52PowerState  nRF52State;
	
    struct _scanchannel{
        unsigned char ScanChannel[3];   /* fixed channel */
        unsigned char CurChannel;       /* 0 ~ 2 */
        unsigned int ScanTimeout;
        __IO unsigned int Broadcasts;
        unsigned int NextBroadcastTime;
    }ScanChannel;

    unsigned int NextSendTime;
    
    unsigned int SampleBufPointer;
    unsigned int SampleBufLen;
    __IO unsigned int SampleCurPos;
    unsigned int SampleSendPos;
    unsigned int DenoisePos;
    int SampleFrameQuietCnt;

    unsigned int RfBufPointer;
    unsigned int RfBufLen;

    struct _pair{
        unsigned char PairHeader[10];        /* 0xA55A + "DM" + 0x5AA5 + Addr */
        unsigned char WorkChannel;
        unsigned char Rate;
        unsigned char AdcBits;
        unsigned char FpwmDiv;
        unsigned int UsedChannelBitMaps[4];  /* from 2400 ~ 2500 */
    }Pair;
    
    __IO BroadcastType brdcstType;
    
    uint32_t deviceId;
    uint32_t txAddress;
    DevicesInfo deviceInfo;
    uint32_t connectedDeviceId;   //For bond control
		
    __IO unsigned int FlagRfRecv;
    __IO EventInfo eventInfo;
    
    Rftest rft;
}SenderSysCfg;

extern SenderSysCfg SenderSC;

void Broadcast_SetType(BroadcastType iType, EAudioEvent ev);
    
unsigned int RND_Get(void);
void Watchdog_Init( uint32_t crvSeconds );
void Watchdog_Feed(void);
void SystemClock_Init(void);
void SystemTick_Init(void);
unsigned int SystemTick_Get(void);
unsigned int SystemTick_GetTicks(void);
unsigned int SystemTick_GetSecond(void);

void PostMessage(Message id);
Message GetMessage(void);
void MessageProcess(Message msgId);

void BSP_GpioInit(void);
void Delay_ms( uint32_t ms );
void MicStateUpdate(void);
bool AudioLineIsChanged(void);
#ifdef AT_FUNCTIONS
void SimCardUpdate(void);
bool SimCardDetect(void);
#endif
BattGrade BatteryLevelDetect(void);
BattGrade BatteryGetGrade(void);
uint16_t BatteryGetValue(void);
void Sample_Init(void);
void Sample_Start(void);
void Sample_Stop(void);
void NoiseSample_Start(void);
void NoiseSample_Stop(void);
void Noise_Init(void);

void Rf_Init(void);
void Rf_SetMode(int mode);
void Rf_TestStart( uint8_t Channel, bool Dir, uint32_t Duration);
bool Rf_Test( void );
void Rf_StartRx(uint32_t bufPointer, uint32_t len, uint8_t channel);
void Rf_Broadcast(uint32_t bufPointer, uint32_t len, uint8_t channel, uint8_t type, uint32_t dstDevice );
int  Rf_SendPackage(uint32_t bufPointer, uint32_t len, uint8_t channel);
void Rf_ReceiveUpdate(void);
void Rf_AddressUpdate(void);

void PAM_Power( bool bOn );

int IMA_ADPCM_BlockEncode(short * indata, unsigned char * outdata, int len);

void SignalProcess_Init(signed short * noise,int frameNumber);
void SignalProcess_Run(signed short * SrcSignalArray,const unsigned int SrcSignalArrayLen,unsigned int * SrcSignalArrayPos);

bool IsQuietFrameEx(signed short * SrcSignalArray, int len);
#endif

