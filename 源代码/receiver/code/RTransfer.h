#ifndef _TRANSMIT_H_
#define _TRANSMIT_H_

#include "Queue.h"
#include "stdbool.h"

typedef enum
{
    TRANS_NORMAL = 0x00,
    TRANS_STACK_FULL = 0x81,
    TRANS_ERROR_EMPTY,
    TRANS_ERROR_STACK,
    TRANS_ERROR_SECURITY,
    TRANS_ERROR_ADDRESS,
}trans_status;

/******************************************************************
 *All functions are listed for transmission layer.
 */
void rxTransferInit(void);

int  rxFrameProcess(unsigned char* in, signed short * out);

#endif
