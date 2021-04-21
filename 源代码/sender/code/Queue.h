/*******************************************************
 * Queue.h
 *
 * Created: 2018/07/09
 *  Author: Lamy Xiang
 *******************************************************/ 

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "Sender.h"

#define AUDIO_FRAME_SIZE  (((DATALEN_PER_FRAME+6)+15)/16*16)
#define AUDIO_QUEUE_SIZE  48

#define UART_FRAME_SIZE   32
#define UART_QUEUE_SIZE   4

#define MSG_FRAME_SIZE    1
#define MSG_QUEUE_SIZE    20

typedef enum
{
    DATA_VALID = 0,
    DATA_INVALID,
}data_validity;

// overwritten strategy
typedef enum
{
    STACK_NORMAL = 0,
    STACK_FULL,  
    STACK_EMPTY,
    STACK_ERROR,
}stack_status;

typedef enum
{
    QUEUE_UART = 0,
    QUEUE_AUDIO,
    QUEUE_UNKNOWN,
}QUEUE_ID;


typedef struct
{
    uint8_t evt:3;
    uint8_t mode:1;
    uint8_t type:2;
    uint8_t repeat:2;
}DataType;

// PCM Frame
typedef struct
{
    uint8_t dev[4];
    uint8_t id;
    union
    {
       DataType dtype;
       uint8_t  value;
    }info;
    uint8_t data[AUDIO_FRAME_SIZE-6]; 
}PCMFrame;


//Frame
typedef struct
{
    int size;
    uint8_t *data; 
}Frame;

// Queue
typedef struct
{
    __IO int QHead;
    __IO int QTail;
    int  fsize;
    int  qsize;
    Frame *Frame;
}Queue;

extern Queue uartQ, audioQ, msgQ;

int getQueueElements(Queue *q);
data_validity isQueueNotEmpty(Queue *q);
data_validity isQueueFull(Queue *q);
stack_status pushQueue(Queue *q, const Frame* frame);
stack_status pullQueue(Queue *q, Frame* frame);
void emptyQueue(Queue *q);

uint32_t initQueue( void );

#endif /* _QUEUE_H_ */
