#ifndef _S_TRANSFER_H_
#define _S_TRANSFER_H_

#include <stdbool.h>
#include "Queue.h"

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
void TransferReset( void );

void broadcastProcess( char *info, int len, unsigned char channel );
void rawDataProcess( short *source, int len, FTYPE fType, EAudioEvent ev);
void FrameProcess( void );

#endif
