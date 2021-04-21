/* Copyright (c) 2018 Tomore Semiconductor. All Rights Reserved.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#ifndef __SENDER_BSP_H__
#define __SENDER_BSP_H__

//Hardware version control.
#define BOARD_0       0   //Green board/( LD bond board ) which supports TWIS for bond operation and only 1 ADC channel.
#define BOARD_1       1   //Blue board which supports 2 ADC channels(AUDIO and noise separately sample)
#define BOARD_2       2   //AT small green board which supports SIM800C(Uart), Battery level detection and Button operation. 
#define BOARD_3       3   //AT big green board which supports SIM800C(Uart), Battery level detection and Button operation. 
#define BOARD_4       4   //AT big black board which supports SIM800C(Uart), Battery level detection and Button operation. 
#define BOARD_5       5   //Bluetooth black board which supports Battery level detection and Button operation. 
#define BOARD_6       6   //AT big black board which change RFX2411 to RFX2402E, an ADC for audio and noise on basing on BOARD_4. 
#define BOARD_7       7   //Bluetooth green board which supports Battery level detection, Button operation and RFX2402E. 
#define HARD_BOARD    BOARD_7

#if (HARD_BOARD == BOARD_1)
#define AUDIO_SRC_SELECT_PIN     11
#define NOISE_SAMPLE_SWITCH      6
#define SAADC_CHANNEL_NUMBER     1
#elif (HARD_BOARD == BOARD_2)
#define AUDIO_SRC_SELECT_PIN     25
#define NOISE_SAMPLE_SWITCH      27
#define SAADC_CHANNEL_NUMBER     2
#define BATTERY_LEVEL_DETECT
#elif (HARD_BOARD == BOARD_3)
#define AUDIO_SRC_SELECT_PIN     27
#define NOISE_SAMPLE_SWITCH      29
#define SAADC_CHANNEL_NUMBER     2
#define BATTERY_LEVEL_DETECT
#elif (HARD_BOARD == BOARD_4)
#define AUDIO_SRC_SELECT_PIN     1
#define NOISE_SAMPLE_SWITCH      2
#define SAADC_CHANNEL_NUMBER     2
#define BATTERY_LEVEL_DETECT
#elif (HARD_BOARD == BOARD_5 || HARD_BOARD == BOARD_7)
#define AUDIO_SRC_SELECT_PIN     31
#define NOISE_SAMPLE_SWITCH      2
#define SAADC_CHANNEL_NUMBER     2
#define BATTERY_LEVEL_DETECT
#elif (HARD_BOARD == BOARD_6)
#define AUDIO_SRC_SELECT_PIN     3
#define NOISE_SAMPLE_SWITCH      7
#define AUDIO_SAMPLE_SWITCH      8
#define SAADC_CHANNEL_NUMBER     2
#define BATTERY_LEVEL_DETECT
#else
#define SAADC_CHANNEL_NUMBER     1
#endif

#if (HARD_BOARD == BOARD_5 || HARD_BOARD == BOARD_7)
#define ADC_SAMPLE_DATA          3  //Audio In ADC
#define ADC_SAMPLE_MIC           2  //Mic/Noise ADC
#define ADC_SAMPLE_BATTERY       5  //Battery ADC
#elif (HARD_BOARD == BOARD_6)
//Select adc source by 2 SWITCH pins
#define ADC_SAMPLE_DATA          2   //Audio in
#define ADC_SAMPLE_BATTERY       7   //Battery Detect
#else
#define ADC_SAMPLE_DATA          2   //Audio in
#define ADC_SAMPLE_MIC           1   //Mic/NOISE in
#define ADC_SAMPLE_BATTERY       3   //Battery Detect
#endif

#if (HARD_BOARD == BOARD_2 || HARD_BOARD == BOARD_3 || HARD_BOARD == BOARD_4 || HARD_BOARD == BOARD_6) 
#if (HARD_BOARD == BOARD_2)
//SIM power
#define SIM800_POWER_PIN         14      

//POWER KEY
#define BUTTON_POWER_PIN         26      

//Function button
#define BUTTON_SELECT_PIN        28

//LED Pin
#define LED0_OUT_PIN             31

//SIM800C State Pin
#define SIM800C_STATE_PIN        8
#endif

#if  (HARD_BOARD == BOARD_3)
//SIM power
#define SIM800_POWER_PIN         15      

//POWER KEY
#define BUTTON_POWER_PIN         25      

//Function button
#define BUTTON_SELECT_PIN        26

//LED Pin
#define LED0_OUT_PIN             28
#define LED1_OUT_PIN             30
#define LED2_OUT_PIN             31

//Charge Pin
#define CHARGING_PIN             2
#define STANDBY_PIN              1

//SIM800C State Pin
#define SIM800C_STATE_PIN        14

//SIM detect pin
#define SIMCARD_DETECT_PIN       16
#endif

#if  (HARD_BOARD == BOARD_4)
//SIM power
#define SIM800_POWER_PIN         15      

//POWER KEY
#define BUTTON_POWER_PIN         30      

//Function button
#define BUTTON_SELECT_PIN        31

//LED Pin
#define LED0_OUT_PIN             25
#define LED1_OUT_PIN             26
#define LED2_OUT_PIN             27

//Charge Pin
#define CHARGING_PIN             28
#define STANDBY_PIN              29

//SIM800C State Pin
#define SIM800C_STATE_PIN        14

//SIM detect pin
#define SIMCARD_DETECT_PIN       16
#endif

#if  (HARD_BOARD == BOARD_6)
//SIM power
#define SIM800_POWER_PIN         17      

//POWER KEY
#define BUTTON_POWER_PIN         1      

//Function button
#define BUTTON_SELECT_PIN        2

//LED Pin
#define LED0_OUT_PIN             26
#define LED1_OUT_PIN             27
#define LED2_OUT_PIN             28

//Charge Pin
#define CHARGING_PIN             29
#define STANDBY_PIN              30

//SIM800C State Pin
#define SIM800C_STATE_PIN        16

//SIM detect pin
#define SIMCARD_DETECT_PIN       20
#endif

//For button definitions.
#define BUTTON_ADD               9
#define BUTTON_MINUS             10

#if ( HARD_BOARD == BOARD_6 )
//For SIM800
#define AT_UART_RI_PIN           13
#define AT_UART_TX_PIN           14
#define AT_UART_RX_PIN           15
#else
//For SIM800
#define AT_UART_RI_PIN           11
#define AT_UART_TX_PIN           12
#define AT_UART_RX_PIN           13
#endif

#define BUTTON_FUNCTIONS
#define AT_FUNCTIONS
#define VOLUME_ADJUST

#endif


#if (HARD_BOARD == BOARD_5) 
//POWER BLUETOOTH
#define BLUETOOTH_POWER_PIN      8      

//POWER KEY
#define BUTTON_POWER_PIN         11      

//Function button
#define BUTTON_SELECT_PIN        30

//LED Pin
#define LED0_OUT_PIN             14
#define LED1_OUT_PIN             13
#define LED2_OUT_PIN             12

//Charge Pin
#define CHARGING_PIN             28
#define STANDBY_PIN              27

//For button definitions.
#define BUTTON_ADD               9
#define BUTTON_MINUS             10

#define BUTTON_FUNCTIONS
#define VOLUME_ADJUST
#endif

#if (HARD_BOARD == BOARD_7) 
//POWER BLUETOOTH
#define BLUETOOTH_POWER_PIN      8      

//POWER KEY
#define BUTTON_POWER_PIN         13      

//Function button
#define BUTTON_SELECT_PIN        30

//LED Pin
#define LED0_OUT_PIN             16
#define LED1_OUT_PIN             15
#define LED2_OUT_PIN             14

//Charge Pin
#define CHARGING_PIN             28
#define STANDBY_PIN              27

//For button definitions.
#define BUTTON_ADD               9
#define BUTTON_MINUS             10

#define BUTTON_FUNCTIONS
#define VOLUME_ADJUST
#endif

#ifdef SERIAL_DEBUG
#if  (HARD_BOARD == BOARD_3 || HARD_BOARD == BOARD_4  || HARD_BOARD == BOARD_5 || HARD_BOARD == BOARD_7)
//For debug
#define UART_TX_PIN              17 
#define UART_RX_PIN              18
#elif ( HARD_BOARD == BOARD_6 )
#define UART_TX_PIN              18 
#define UART_RX_PIN              19
#else
//For debug
#define UART_TX_PIN              15 
#define UART_RX_PIN              16
#endif
#endif

#if( HARD_BOARD == BOARD_0 )
//For Green board embeded phone board(LD bond board).
#define TWIS_SCL_S               8
#define TWIS_SDA_S               1
#elif defined(TWIS_DEBUG) 
#define TWIS_SCL_S               15
#define TWIS_SDA_S               16
#endif

//For RFX2411N
#if( HARD_BOARD == BOARD_3 || HARD_BOARD == BOARD_4  || HARD_BOARD == BOARD_5 )
#define PA_PIN_TXEN        24
#define PA_PIN_RXEN        23
#define PA_PIN_PDET        22
#define PA_PIN_MODE        20
#define PA_PIN_SWANT       19
#elif( HARD_BOARD == BOARD_2 || HARD_BOARD == BOARD_1 )
#define PA_PIN_TXEN        21
#define PA_PIN_RXEN        20
#define PA_PIN_MODE        18
#define PA_PIN_SWANT       17
#elif( HARD_BOARD == BOARD_6 )
#define PA_PIN_PDET        22
#define PA_PIN_RXEN        23
#define PA_PIN_TXEN        24
#define PA_PIN_POWER       25
#elif( HARD_BOARD == BOARD_7 )
#define PA_PIN_PDET        22
#define PA_PIN_RXEN        23
#define PA_PIN_TXEN        24
#define PA_PIN_POWER       26
#else
#define PA_PIN_TXEN        21
#define PA_PIN_RXEN        20
#define PA_PIN_MODE        19
#define PA_PIN_SWANT       18
#endif


#endif // __SENDER_BSP_H__
