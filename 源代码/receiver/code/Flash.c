#include "Flash.h"
#include "Receiver.h"

/**@brief Function for erasing a page in flash.
 *
 * @param page_address Address of the first word in the page to be erased.
 */
static void flash_page_erase(uint32_t * page_address)
{
    // Turn on flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Erase page:
    NRF_NVMC->ERASEPAGE = (uint32_t)page_address;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Turn off flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
}


/**@brief Function for filling a page in flash with a value.
 *
 * @param[in] address Address of the first word in the page to be filled.
 * @param[in] value Value to be written to flash.
 */
static void flash_word_write(uint32_t * address, uint32_t value)
{
    // Turn on flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    *address = value;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Turn off flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
}


/**
 * @brief Function for application GetBindingNum entry.
 */
void UpdateDeviceInfo(void)
{
    uint32_t   bondedId = 0;
    uint32_t * addr;
    uint32_t   i = 0;
    uint32_t   pg_size;
    uint32_t   pg_num;

    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 2;  // Use last second page in flash

    //Start address:
    addr = (uint32_t *)(pg_size *pg_num);
    while(*addr != (uint32_t)-1){
        ReceiverSC.deviceId = *addr++;
        bondedId = *addr++;
        i += 8;
        if (i>=pg_size) break;
    }    

    if ( ReceiverSC.deviceId == 0 ){
        ReceiverSC.deviceId = NRF_FICR->DEVICEID[0];		
    }    
    ReceiverSC.boundedDeviceId = bondedId;
}

/**
 * @brief Function for application SetBondedNum entry.
 */
void SetBondedDevice( uint32_t device )
{
    uint32_t * addr;
    uint32_t   i = 0;
    uint32_t   pg_size;
    uint32_t   pg_num;

    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 2;  // Use last second page in flash

    // Start address:
    addr = (uint32_t *)(pg_size *pg_num);
    while(*addr != (uint32_t)-1){
        addr+=2;
        i += 8;
        if (i>=pg_size) break;
    }    
    
    if ( i>=pg_size ){
        addr = (uint32_t *)(pg_size * pg_num);
        flash_page_erase(addr);    
    }    
    
    if ( ReceiverSC.deviceId == 0 ){
        ReceiverSC.deviceId = NRF_FICR->DEVICEID[0];		
    }
    flash_word_write(addr++, ReceiverSC.deviceId);
    flash_word_write(addr++, device);
    ReceiverSC.boundedDeviceId = device;
}

