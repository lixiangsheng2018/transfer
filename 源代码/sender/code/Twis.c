/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "nordic_common.h"
#include "nrf_drv_twis.h"
#include "nrf_gpio.h"
#include "app_util_platform.h"
#include "Twis.h"
#include "Flash.h"
#include "Sender.h"

#if defined(TWIS_DEBUG) && defined(SERIAL_DEBUG)
#define TWISLog(...) printf(__VA_ARGS__)
#else
#define TWISLog(...)
#endif

#if defined(TWIS_SCL_S) && defined(TWIS_SDA_S)	
/**
 * Internal variables required for the module to work.
 */

/**
 * @brief Memory array
 *
 * Place that would simulate our memory
 */
static uint8_t m_memory[TWIS_BUFFER_SIZE];

/**
 * @brief Current memory address
 *
 * Memory pointer for any operation that would be processed on memory array.
 */
static size_t m_addr = 0;

/**
 * @brief Receive buffer
 *
 * Receiving buffer has to contain address and 8 bytes of data
 */
static uint8_t m_rxbuff[1 + TWIS_SEQ_WRITE_MAX];

/**
 * @brief Internal error flag
 *
 * This flag is set if any communication error is detected.
 * It would be cleared by calling the @ref eeprom_simulator_error_get function.
 */
static bool m_error_flag;

/**
 * @brief TWIS instance
 *
 * TWIS driver instance used by this EEPROM simulator
 */
static const nrf_drv_twis_t m_twis = NRF_DRV_TWIS_INSTANCE(TWIS_INST);

/**
* Internal auxiliary functions
*/
/**
 * @brief Set current address pointer
 *
 * Sets address for the next operation on the memory array
 * @param addr Address to set
 */
static void TWIS_setAddr(size_t addr)
{
    m_addr = addr;
}

/**
 * @brief Perform write operation on memory array
 *
 * Write single byte into memory array using @ref m_addr as a write pointer.
 * @param data Data to be written
 */
static void TWIS_write(uint8_t data)
{
    if(m_addr >= sizeof(m_memory)){
        m_addr = 0;
    }
    
    //It is read only for those REGs smaller than TWIS_REG_VOLUME.
    if ( m_addr > TWIS_REG_BONDED_DEVICE_END && m_addr != TWIS_REG_MCU_STATE ){
        //Save command.
        m_memory[m_addr] = data;
    }
    
    switch(m_addr){
        case TWIS_REG_CMD:{
            switch(data){    
            case TWIS_CMD_BOND: //For bonding.
                if ( m_memory[0] < MAX_BONDED_DEVICES ){
                    Broadcast_SetType( BROADCAST_BOND, AUDIO_EVENT_RECONNECT );
                }
                break;
            case TWIS_CMD_UNBOND: //For unbonding.
                Broadcast_SetType( BROADCAST_UNBOND, AUDIO_EVENT_RECONNECT );
                break;
            case TWIS_CMD_CLEAR:  //Clear bonding information.
                ClearBondedDevices();
                Broadcast_SetType( BROADCAST_CLEAR, AUDIO_EVENT_RECONNECT );
                memset(m_memory, 0x00, 33);
                break;
#if (UNBOUND_FORCE)
            case TWIS_CMD_CLEAR_DEVICE:  //Clear bonded device information.
                Broadcast_SetType( BROADCAST_UNBOND_FORCE, AUDIO_EVENT_RECONNECT );
                break;
#endif						
            default:
                TWISLog("Unknow twis cmd:%d\n", m_addr);
                break;
            }
        }break;
        case TWIS_REG_VOLUME:{
            if ( (uint8_t)SenderSC.deviceInfo.volume != data ){
                if (data<MIN_VOLUME) SenderSC.deviceInfo.volume = MIN_VOLUME;
                else if (data>MAX_VOLUME)  SenderSC.deviceInfo.volume = MAX_VOLUME;
                else SenderSC.deviceInfo.volume = data;
                UpdateDevicesInfo();
            }
        }break;
        case TWIS_REG_DATA:{
            SenderSC.deviceInfo.datatype = data;
        }break;
        case TWIS_REG_ADDRESS_ID:{  //Set device address Id.
            uint32_t addressId = (data?1:0);
            if ( SenderSC.deviceInfo.addressId != addressId ){
                SenderSC.deviceInfo.addressId = addressId;
                Broadcast_SetType( BROADCAST_ADDRESS_CHANGED, AUDIO_EVENT_RECONNECT );
            }
        }break;
        default: break;
    }
    
    TWISLog("TWIS_write:%d %d\n", m_addr, data);
    m_addr++;    
}

/**
 * @brief Start after WRITE command
 *
 * Function sets pointers for TWIS to receive data
 * WRITE command does not write directly to memory array.
 * Temporary receive buffer is used.
 * @sa m_rxbuff
 */
static void TWIS_writeBegin(void)
{
    (void)nrf_drv_twis_rx_prepare(&m_twis, m_rxbuff, sizeof(m_rxbuff));
}

/**
 * @brief Finalize WRITE command
 *
 * Function should be called when write command is finished.
 * It sets memory pointer and write all received data to memory array
 */
static void TWIS_writeEnd(size_t cnt)
{
    if(cnt > 0){
        size_t n;
        TWIS_setAddr(m_rxbuff[0]);
        for(n=1; n<cnt; ++n){
            TWIS_write(m_rxbuff[n]);
        }
    }
}


/**
 * @brief Start after READ command
 *
 * Function sets pointers for TWIS to transmit data from current address to the end of memory.
 */
static void TWIS_readBegin(void)
{
    if(m_addr >= sizeof(m_memory)){
        m_addr = 0;
    }
    
    (void)nrf_drv_twis_tx_prepare(&m_twis, m_memory+m_addr,  sizeof(m_memory)-m_addr);
}

/**
 * @brief Finalize READ command
 *
 * Function should be called when read command is finished.
 * It adjusts current m_addr pointer.
 * @param cnt Number of bytes readed
 */
static void TWIS_readEnd(size_t cnt)
{
    m_addr += cnt;
}


/**
 * @brief Event processing
 *
 *
 */
static void TWIS_event_handler(nrf_drv_twis_evt_t const * const p_event)
{
    switch(p_event->type)
    {
    case TWIS_EVT_READ_REQ:
        if(p_event->data.buf_req){
            TWIS_readBegin();
        }
        break;
    case TWIS_EVT_READ_DONE:
        TWIS_readEnd(p_event->data.tx_amount);
        break;
    case TWIS_EVT_WRITE_REQ:
        if(p_event->data.buf_req){
            TWIS_writeBegin();
        }
        break;
    case TWIS_EVT_WRITE_DONE:
        TWIS_writeEnd(p_event->data.rx_amount);
        break;

    case TWIS_EVT_READ_ERROR:
    case TWIS_EVT_WRITE_ERROR:
    case TWIS_EVT_GENERAL_ERROR:
        m_error_flag = true;
        break;
    default:
        break;
    }
}

void TWIS_DevicesUpdate(void)
{
    int i;

    m_memory[0] = MIN(MAX_BONDED_DEVICES, SenderSC.deviceInfo.bondedNum);

    for(i=0; i<MAX_BONDED_DEVICES; i++){
        uint8_t *dev = (uint8_t *)&SenderSC.deviceInfo.bondedDevices[i];
        m_memory[i*4+1] = dev[0];       
        m_memory[i*4+2] = dev[1];       
        m_memory[i*4+3] = dev[2];       
        m_memory[i*4+4] = dev[3];       
    }
}

bool TWIS_VolumeUpdate(void)
{
    if ( m_memory[TWIS_REG_VOLUME] != SenderSC.deviceInfo.volume){
        m_memory[TWIS_REG_VOLUME] = SenderSC.deviceInfo.volume;
        return true;
    }

    return false;
}

void TWIS_McuStateUpdate(void)
{
    if ( m_memory[TWIS_REG_MCU_STATE] != SenderSC.state){
        m_memory[TWIS_REG_MCU_STATE] = SenderSC.state;
    }
}

void TWIS_DataUpdate(void)
{
    TWIS_DevicesUpdate();
    TWIS_VolumeUpdate();	  
    m_memory[TWIS_REG_ADDRESS_ID] = SenderSC.deviceInfo.addressId;
    m_memory[TWIS_REG_MCU_STATE] = SenderSC.state;
}

ret_code_t TWIS_Init(void)
{
    ret_code_t ret;

  	const nrf_drv_twis_config_t config =
    {
        .addr               = {TWIS_DEVICE_ADDR, 0},
        .scl                = TWIS_SCL_S,
        .sda                = TWIS_SDA_S,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH
    };

    /* Initialize data with pattern */
    TWIS_DataUpdate();
    m_addr = 0;

    /* Init TWIS */
    do{
        ret = nrf_drv_twis_init(&m_twis, &config, TWIS_event_handler);
        if(NRF_SUCCESS != ret){
            break;
        }

        nrf_drv_twis_enable(&m_twis);
    }while(0);

    return ret;
}


bool TWIS_Error_check(void)
{
    return m_error_flag;
}

uint32_t TWIS_Error_get_and_clear(void)
{
    uint32_t ret = nrf_drv_twis_error_get_and_clear(&m_twis);
    m_error_flag = false;
    return ret;
}

void TWIS_Dump( TWIS_REG reg )
{
    switch(reg)
    {
    case TWIS_REG_BONDEDNUM:
        Log("Bonded num:%d\n", (uint16_t)m_memory[0]);	   
        break;
    case TWIS_REG_VOLUME:
        Log("VOL:%02x\n", (uint16_t)m_memory[reg]);	   
        break;
    case TWIS_REG_CMD:
        Log("CMD:%02x\n", (uint16_t)m_memory[reg]);	   
        break;
    case TWIS_REG_DATA:
        Log("Data Type:%02x\n", (uint16_t)m_memory[reg]);	   
        break;
    case TWIS_REG_ADDRESS_ID:
        Log("Sel Id:%02x\n", (uint16_t)m_memory[reg]);	   
        break;
    default:
       if ( reg>= TWIS_REG_BONDED_DEVICE_START && reg <= TWIS_REG_BONDED_DEVICE_END){
       #ifdef SERIAL_DEBUG
             uint16_t devId = reg/4*4;       
             Log("Bond DID:%02x%02x%02x%02x", m_memory[devId+1],       
                                              m_memory[devId+2],
                                              m_memory[devId+3],
                                              m_memory[devId+4]);					 
       #endif
       }else{
             uint16_t i;
             Log("EEPROM:\n");
             for (i=0;i<TWIS_BUFFER_SIZE;i++){
                 Log("%02x ", m_memory[i]);					 
                 if ( i%8 == 7) Log("\n");
             }
             Log("\n");
        }
    }
}

#else
ret_code_t TWIS_Init(void)
{
    return (ret_code_t)-1;    
}

void TWIS_DevicesUpdate(void)
{
}

bool TWIS_VolumeUpdate(void)
{
    return false;
}

void TWIS_Dump(TWIS_REG reg)
{
}

void TWIS_McuStateUpdate(void)
{
}
#endif    
