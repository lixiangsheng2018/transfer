#ifndef __FLASH_HEARDER__
#define __FLASH_HEARDER__

#include "nrf.h"

/**
 * @brief Function for application GetBondedDevices entry.
 */
void UpdateDeviceInfo(void);

/**
 * @brief Function for application SetBondedDevices entry.
 */
void SetBondedDevice(uint32_t device);

#endif
