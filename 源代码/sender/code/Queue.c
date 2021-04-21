/*************************************************************
 * Queue.c
 *
 * Copyright: Tomore
 * Created: 2018/07/09
 *  Author: Lamy Xiang
 *************************************************************/

#include "Queue.h"

Queue uartQ, audioQ, msgQ;

int getQueueElements(Queue *q)
{
    return (q->qsize + q->QHead - q->QTail)%q->qsize; 
}

data_validity isQueueNotEmpty(Queue *q)
{
    if(q->QHead != q->QTail){
        return DATA_VALID;
    } else {
        return DATA_INVALID;
    }
}

data_validity isQueueFull(Queue *q)
{
    if(((q->QHead + 1)%q->qsize) == q->QTail){
        return DATA_VALID;
    } else {
        return DATA_INVALID;
    }
}

stack_status pushQueue(Queue *q, const Frame* frame)
{
    //入队列采用覆盖式，最新的Frame覆盖旧Frame
    int i;

    q->Frame[q->QHead].size = frame->size;
    for (i = 0; i < frame->size; i++) {
        q->Frame[q->QHead].data[i] = frame->data[i];
    }

    q->QHead = (q->QHead+1)%q->qsize;

    if (q->QHead == q->QTail) {
        // head catch the tail
        q->QTail = (q->QTail+1)%q->qsize;
    }

    return STACK_NORMAL;
}

stack_status pullQueue(Queue *q, Frame * frame)
{
    if ( q->QHead != q->QTail ) {
        int size = q->Frame[q->QTail].size;
        if ( size <= q->fsize ){
            if (frame != NULL){
                int i;
                uint8_t *pframe = frame->data;
                uint8_t *pqueue = q->Frame[q->QTail].data;

                frame->size = size;
                for (i = 0; i < size; i++) {
                    pframe[i] = pqueue[i];
                }
            }

            q->QTail = (q->QTail+1)%q->qsize;
        
            return STACK_NORMAL;
        }else{
            return STACK_ERROR;
        }
    }else{
        return STACK_EMPTY;
    }
}

void emptyQueue(Queue *q)
{
    q->QHead = 0;
    q->QTail = 0;
}

uint32_t initQueue(void)
{
    static bool bInited = false;
    
    int i;
    uint32_t errorcode = 0;

    //Initialize the header of queue.    
    uartQ.QHead = 0;
    uartQ.QTail = 0;
    
    audioQ.QHead = 0;
    audioQ.QTail = 0;

    msgQ.QHead = 0;
    msgQ.QTail = 0;
	
    if(!bInited){ 
        uint8_t *lpBuf;
        bInited = true;
      
        //Initialize queue buffer.        
        //For uart queue.
        uartQ.qsize = UART_QUEUE_SIZE;
        uartQ.fsize = UART_FRAME_SIZE;
        lpBuf = malloc(uartQ.qsize*sizeof(Frame));
        uartQ.Frame = (Frame *)lpBuf;
        if ( lpBuf != NULL ){
            lpBuf = malloc(uartQ.qsize*uartQ.fsize);
            if ( lpBuf != NULL ){
                for ( i=0; i<uartQ.qsize; i++ ){
                    uartQ.Frame[i].data = lpBuf + i*uartQ.fsize;
                }
            }else{
                errorcode = 0x8001;
            }
        }else{
            errorcode = 0x8000;
        }
    
        //For audio queue.
        audioQ.qsize = AUDIO_QUEUE_SIZE;
        audioQ.fsize = AUDIO_FRAME_SIZE;
        lpBuf = malloc(audioQ.qsize*sizeof(Frame));
        audioQ.Frame = (Frame *)lpBuf;
        if ( lpBuf != NULL ){
            lpBuf = malloc(audioQ.qsize*audioQ.fsize);
            if ( lpBuf != NULL ){
                for ( i=0; i<audioQ.qsize; i++ ){
                    audioQ.Frame[i].data = lpBuf + i*audioQ.fsize;
                }    
            }else{
                errorcode = 0xC001;            
            }
        }else{
            errorcode = 0xC000;            
        }
				
        //For message queue.
        msgQ.qsize = MSG_QUEUE_SIZE;
        msgQ.fsize = MSG_FRAME_SIZE;
        lpBuf = malloc(msgQ.qsize*sizeof(Frame));
        msgQ.Frame = (Frame *)lpBuf;
        if ( lpBuf != NULL ){
            lpBuf = malloc(msgQ.qsize*msgQ.fsize);
            if ( lpBuf != NULL ){
                for ( i=0; i<msgQ.qsize; i++ ){
                    msgQ.Frame[i].data = lpBuf + i*msgQ.fsize;
                }    
            }else{
                errorcode = 0xC001;            
            }
        }else{
            errorcode = 0xC000;            
        }
    }
    
    return errorcode;
}
