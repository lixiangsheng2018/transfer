#ifndef __AT_H__
#define __AT_H__

#include <stdbool.h>
#include <stdint.h>

#define APP_TIMER_PRESCALER     0     /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE 2     /**< Size of timer operation queues. */

typedef enum{
    PAS_INITING,
    PAS_SIMCS,
    PAS_GETLOV,
    PAS_IDLE,
    PAS_RING,
    PAS_CONNECTED,
    PAS_SETVOL,
    PAS_UNKOWN
}PAStatus;

typedef enum{
    ATMSG_IDLE,
    ATMSG_LOW_BATTERY,
    ATMSG_POWER_KEY,
    ATMSG_RESPONSE,
    ATMSG_AUDIO_CHANGED,
}ATMsg;

typedef struct _AT_CMD {
    char *cmd;
    char *response;
    uint8_t wait_time;
}ATCmd;

typedef struct _BUTTON_INFO {
    uint32_t pin;
    uint32_t bPressed;
    uint32_t interval;
    uint32_t elapseTM;
}ButtonInfo;

/********************
 * AT command.
 */
void AT_Command( char *b, char *a, uint8_t wait_time);

/**
 * @brief SIM card configuring
 */
void AT_SimInit(void);

/**
 * @brief Function for power switch nRF52832/SIM800C modules
 */
void AT_PowerOn( bool on );

void SIM800C_PowerOn( bool on );

#endif
