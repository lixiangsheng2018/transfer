#ifndef _DEFINES_H_
#define _DEFINES_H_
#include "bsp.h"

#define FREQ_LOW                   1
#define FREQ_HIGH                  0
#define FREQ_RANGE                 FREQ_HIGH  

#define DENOISE_NONE               0 
#define DENOISE_DEFAULT            1  //Spectral Substraction
#define DENOISE_SS_1               2  //Self-adaptive Spectral Substraction
#define DENOISE_WEBRTC             3  //Webrtc noise suppression

#define DENOISE_PROCESS            DENOISE_DEFAULT

#define UNBOUND_FORCE              0
#if (UNBOUND_FORCE)
#ifdef  AT_FUNCTIONS
#undef  AT_FUNCTIONS  
#endif
#endif

#define AUTO_POWERON_ON_CHARGING   0

#define TRANSFER_AUTO              0
#define TRANSFER_AFTER_PROCESSED   1
#define TRANSFER_MODE              TRANSFER_AFTER_PROCESSED

//Definition for fixed channel if MAX_CHANNEL_NO is 1.
#define MAX_CHANNEL_NO             3

#define WHITE_NOISE_LEVEL          0
#define MAX_REPEAT                 1

//Keep the same frame size(DATALEN_PER_FRAME) as that of receiver.
#if (DENOISE_PROCESS == DENOISE_WEBRTC)
#define SAMPLE_RATE                8000
#if (SAMPLE_RATE == 16000)
#define DATALEN_PER_RAWFRAME       160
#define DATALEN_PER_FRAME          84 
#else  //For sample rate 8000
#define DATALEN_PER_RAWFRAME       80
#define DATALEN_PER_FRAME          44 
#endif
#else
#define SAMPLE_RATE                8000
#define DATALEN_PER_RAWFRAME       256
#define DATALEN_PER_FRAME          68 
#endif

#define IMA_ADPCM_4BIT_BLOCK       DATALEN_PER_FRAME
#define IMA_ADPCM_PCM_RAW_LEN      ((IMA_ADPCM_4BIT_BLOCK - 4) * 2)
#define IMA_US2S_BASE_VALUE        32768

#define TRANSFER_BASE              (IMA_ADPCM_PCM_RAW_LEN/(MAX_REPEAT+1))

#define MAX_BONDED_DEVICES         8

#ifndef VOLUME_ADJUST
#define VOLUME_1_0                 192
#define DEFAULT_VOLUME             192
#define MAX_VOLUME                 192
#define MIN_VOLUME                 192
#else
#define MAX_VOLUME                 192
#define MIN_VOLUME                 12
#define VOLUME_1_0                 (MAX_VOLUME)     //(MAX_VOLUME*10/16)
#define DEFAULT_VOLUME             (MIN_VOLUME*4)   //((MIN_VOLUME+MAX_VOLUME)/2)
#endif

#define QUIET_THREHOLD             40 //80
#define VAD_ENABLE                 1

#define NEXT_BOARDCAST_TIME_BASE_NUMBER     8
#if (DENOISE_PROCESS == DENOISE_WEBRTC)
#define MAX_SAMPLE_FRAMES                   24
#define NOISE_SAMPLE_FREAME_NUMBER          200
#else
#define MAX_SAMPLE_FRAMES                   64
#define NOISE_SAMPLE_FREAME_NUMBER          60
#endif

#define BACKWARD_FRAMES                     4
#define FORWARD_FRAMES                      10  // this should be larger than QUIET_SLEEP_MAX_DELAY of receiver.
#define QUIET_START_FRAME_CNT               60  // this should be equal Reciver
#define AUDIO_CACHE_LENGTH                  (DATALEN_PER_RAWFRAME * FORWARD_FRAMES)  
#define MAX_STAT_FRAMES                     (BACKWARD_FRAMES+FORWARD_FRAMES+4)
 
//For battery detect.
#if (HARD_BOARD == BOARD_4)
#define BATTERY_LOW_THRESHOLD               5210  //3.50 
#define BATTERY_MID_LOW_THRESHOLD           5290  //3.55 
#define BATTERY_WARNING_THRESHOLD           5360  //3.60
#define BATTERY_GRADE_1_THRESHOLD           5520  //3.70[3.6~3.7) 
#define BATTERY_GRADE_2_THRESHOLD           5950  //4.00[3.7~4.0] 
#elif (HARD_BOARD == BOARD_5 || HARD_BOARD == BOARD_7)
#define BATTERY_LOW_THRESHOLD               5220  //3.45 
#define BATTERY_MID_LOW_THRESHOLD           5320  //3.50 
#define BATTERY_WARNING_THRESHOLD           5390  //3.55
#define BATTERY_GRADE_1_THRESHOLD           5615  //3.70[3.6~3.7) 
#define BATTERY_GRADE_2_THRESHOLD           6090  //4.00[3.7~4.0] 
#else
#define BATTERY_LOW_THRESHOLD               5190  //3.50 
#define BATTERY_MID_LOW_THRESHOLD           5252  //3.55 
#define BATTERY_WARNING_THRESHOLD           5315  //3.60
#define BATTERY_GRADE_1_THRESHOLD           5615  //<3.80 
#define BATTERY_GRADE_2_THRESHOLD           6065  //<4.1 
#endif
#endif
