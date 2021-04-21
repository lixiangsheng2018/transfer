#include "stdbool.h"
#include "nordic_common.h"
#include "Flash.h"
#include "Sender.h"

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
void GetDevicesInfo( DevicesInfo *info )
{
    uint32_t  *addrTail, *addrHeader;
    uint32_t   i;
    uint32_t   pg_size;
    uint32_t   pg_num;
    
    SenderSC.deviceInfo.size = sizeof(SenderSC.deviceInfo);
    SenderSC.deviceInfo.volume = DEFAULT_VOLUME; 
    SenderSC.deviceInfo.deviceId = NRF_FICR->DEVICEID[0];	

    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 2;  // Use last second page in flash
    addrHeader = (uint32_t *)(pg_size * pg_num);
    addrTail   = addrHeader + (pg_size - SenderSC.deviceInfo.size)/sizeof(uint32_t);
    
    //Search total page.
    while( addrTail >= addrHeader){
        if ( *addrTail != (uint32_t)-1 ){
            if (*addrTail == SenderSC.deviceInfo.size){
                //Get device information, such as bonded number, devices, volume. 
                SenderSC.deviceInfo.bondedNum = MIN(addrTail[1], MAX_BONDED_DEVICES);
                for ( i=0; i<SenderSC.deviceInfo.bondedNum; i++ ){
                    SenderSC.deviceInfo.bondedDevices[i] = addrTail[i+2];
                } 
                if (addrTail[10]<MIN_VOLUME) SenderSC.deviceInfo.volume = MIN_VOLUME;
                else if (addrTail[10]>MAX_VOLUME)  SenderSC.deviceInfo.volume = MAX_VOLUME;
                else SenderSC.deviceInfo.volume = addrTail[10];
                if(addrTail[11] == 1){
                    SenderSC.deviceInfo.addressId = 1;	
                }else{
                    SenderSC.deviceInfo.addressId = 0;	
                }
                if(addrTail[12] != 0 && addrTail[12]!=(uint32_t)-1){
                    SenderSC.deviceInfo.deviceId = addrTail[12];	
                }
                break;
            }
        }
        addrTail -= SenderSC.deviceInfo.size/sizeof(uint32_t);
    }
}

/**
 * @brief Function for application SetBondedNum entry.
 */
void UpdateDevicesInfo(void)
{
    uint32_t  *addrHeader, *addrTail;
    uint32_t   i = 0;
    uint32_t   pg_size;
    uint32_t   pg_num;

    SenderSC.deviceInfo.size = sizeof(SenderSC.deviceInfo);

    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 2;  // Use last second page in flash
    addrHeader = (uint32_t *)(pg_size*pg_num);
    addrTail   = addrHeader + (pg_size - SenderSC.deviceInfo.size)/sizeof(uint32_t);
    
    if ( *addrTail != (uint32_t)-1 ){
        //Erase all page if no new space.
        flash_page_erase(addrHeader); 
        addrTail = addrHeader;       
    }else{    
        //Find the first free room.
        while (*addrTail == (uint32_t)-1)
        {
            addrTail -= SenderSC.deviceInfo.size/sizeof(uint32_t);        
            if ( addrTail < addrHeader) break;
        }      
        addrTail += SenderSC.deviceInfo.size/sizeof(uint32_t);        
    }
    
    if ( addrTail >= addrHeader && *addrTail == (uint32_t)-1){
        //Update parameters to FLASH.
        flash_word_write(&addrTail[0], SenderSC.deviceInfo.size);
        flash_word_write(&addrTail[1], SenderSC.deviceInfo.bondedNum);
        //Set bonded devices.
        for (i=0;i<MIN(8,SenderSC.deviceInfo.bondedNum);i++){
            flash_word_write(&addrTail[2+i], SenderSC.deviceInfo.bondedDevices[i]);
        }
        flash_word_write(&addrTail[10], SenderSC.deviceInfo.volume );
        flash_word_write(&addrTail[11], SenderSC.deviceInfo.addressId );
        flash_word_write(&addrTail[12], SenderSC.deviceInfo.deviceId );
    }
}

/**
 * @brief Function for application SetBondedNum entry.
 */
void ClearBondedDevices( void )
{
    if ( SenderSC.deviceInfo.bondedNum ){
        SenderSC.deviceInfo.bondedNum = 0;
        memset(SenderSC.deviceInfo.bondedDevices, 0x00, sizeof(uint32_t)*MAX_BONDED_DEVICES); 
        UpdateDevicesInfo();    
    }
}

