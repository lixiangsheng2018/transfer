#ifndef __TWIS_H__
#define __TWIS_H__
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
#include "sdk_errors.h"
#include "stdbool.h"

#define TWIS_BUFFER_SIZE    64   //!< Simulated buffer size
#define TWIS_SEQ_WRITE_MAX  8    //!< Maximum number of bytes writable in one sequential access
#define TWIS_DEVICE_ADDR    0x53 //!< Simulated TWI device address

#define TWIS_INST           1    //!< TWIS interface used by EEPROM simulator


typedef enum _TWIS_REG{
    TWIS_REG_BONDEDNUM = 0,
    TWIS_REG_BONDED_DEVICE_START,
    TWIS_REG_BONDED_DEVICE_END = 32,
    TWIS_REG_VOLUME,
    TWIS_REG_CMD,
    TWIS_REG_DATA,
    TWIS_REG_ADDRESS_ID,
    TWIS_REG_MCU_STATE,
}TWIS_REG;

typedef enum _TWIS_CMD{
    TWIS_CMD_BOND = 1,
    TWIS_CMD_UNBOND,
    TWIS_CMD_CLEAR,
    TWIS_CMD_CLEAR_DEVICE,
}TWIS_CMD;

typedef enum _TWIS_DATA{
    TWIS_DATA_NORMAL,
    TWIS_DATA_TEST,
}TWIS_DATA;

void TWIS_DevicesUpdate(void);

bool TWIS_VolumeUpdate(void);

void TWIS_McuStateUpdate(void);

void TWIS_DataUpdate(void);

void TWIS_Dump(TWIS_REG reg);

/**
 * This module simulates the behavior of TWI EEPROM.
 * There are no functions to access internal memory array.
 * Use TWI interface to read or write any data.
 *
 * @attention
 * During initialization EEPROM memory is filled by pattern that is
 * values from 127 downto 0.
 */
ret_code_t TWIS_Init(void);

/**
 * @brief Check if there was any error detected
 *
 * This function returns internal error flag.
 * Internal error flag is set if any error was detected during transmission.
 * To clear this flag use @ref eeprom_simulator_error_get
 * @retval true There is error detected.
 * @retval false There is no error detected.
 */
bool TWIS_Error_check(void);

/**
 * @brief Get and clear transmission error
 *
 * Function returns transmission error data and clears internal error flag
 * @return Error that comes directly from @ref nrf_drv_twis_error_get
 */
uint32_t TWIS_Error_get_and_clear(void);

#endif
