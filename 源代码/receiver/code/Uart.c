#include "Receiver.h"
#include "app_uart.h"
#include "app_error.h"

#ifdef SERIAL_DEBUG
#define UART_TX_BUF_SIZE 256                                                        /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 1                                                          /**< UART RX buffer size. */

void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        //APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
    else if (p_event->evt_type == APP_UART_DATA_READY)
    {
        uint8_t patwr;

        if( app_uart_get(&patwr) != NRF_ERROR_NOT_FOUND){
            printf("%c", patwr);
        }
    }
}

void Uart_Init(void){
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
    {
        UART_RX_PIN,
        UART_TX_PIN,
        (uint8_t)-1,
        (uint8_t)-1,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud115200
    };

    APP_UART_FIFO_INIT(&comm_params,
                        UART_RX_BUF_SIZE,
                        UART_TX_BUF_SIZE,
                        uart_error_handle,
                        APP_IRQ_PRIORITY_LOW,
                        err_code);

    APP_ERROR_CHECK(err_code);       
}

#endif

void rtc_FatalMessage(const char* file, int line, const char* msg)
{
    Log("Fatal error:%s on line %d - %s\n", file, line, msg);
}

void rtc_LogInt( int msg )
{
    Log("RTC msg: %d\n", msg);
}
