#include "Sender.h"
#include "app_uart.h"
#include "app_error.h"
#include "Queue.h"
#include "At.h"

#ifdef SERIAL_DEBUG    

#define UART_TX_BUF_SIZE 256             /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 64              /**< UART RX buffer size. */

uint8_t rxBuf[UART_RX_BUF_SIZE];
uint8_t rxPos = 0;

static void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }    
    else if (p_event->evt_type == APP_UART_DATA_READY)
    {
        uint8_t patwr;

        if( app_uart_get(&patwr) != NRF_ERROR_NOT_FOUND){
            rxBuf[rxPos] = patwr;
            if ( (patwr == '\r')|| (patwr == '\n')){
                if (patwr == '\r' && rxPos>1){
                    Frame uartFrame;
                    rxBuf[rxPos] = 0x00;
                    uartFrame.size = rxPos + 1 ;
                    uartFrame.data = rxBuf;
                    pushQueue(&uartQ, &uartFrame);
                    PostMessage( MSG_UART_RX );
                }                    
                rxPos = 0;
            }else{
                rxPos = (rxPos + 1)%UART_RX_BUF_SIZE;
            }        
        }
    }
}

void Uart_Init( uint8_t rxPin, uint8_t txPin ){
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
    {
        rxPin,
        txPin,
        (uint8_t)-1,
        (uint8_t)-1,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud115200
    };

    app_uart_close();
    APP_UART_FIFO_INIT(&comm_params,
                        UART_RX_BUF_SIZE,
                        UART_TX_BUF_SIZE,
                        uart_error_handle,
                        APP_IRQ_PRIORITY_LOW,
                        err_code);

    APP_ERROR_CHECK(err_code);       
}
#else
void Uart_Init( uint8_t rxPin, uint8_t txPin ){
}
#endif

void rtc_FatalMessage(const char* file, int line, const char* msg)
{
    Log("Fatal error:%s on line %d - %s\n", file, line, msg);
}

void rtc_Log( const char *msg)
{
    Log("%s", msg);
}
