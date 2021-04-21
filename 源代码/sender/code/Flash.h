#ifndef __FLASH_HEARDER__
#define __FLASH_HEARDER__

#include "nrf.h"
#include "Sender.h"

/**
 * @brief Function for application GetBondedDevices entry.
 */
void GetDevicesInfo(DevicesInfo *info);

/**
 * @brief Function for application SetBondedDevices entry.
 */
void UpdateDevicesInfo(void);

/**
 * @brief Function for application ClearBondedDevices entry.
 */
void ClearBondedDevices(void);

#endif
