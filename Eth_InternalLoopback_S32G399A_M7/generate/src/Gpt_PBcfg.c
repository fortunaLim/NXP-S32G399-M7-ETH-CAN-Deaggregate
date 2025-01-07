/*==================================================================================================
* Project : RTD AUTOSAR 4.4
* Platform : CORTEXM
* Peripheral : Stm_Pit_Rtc_Ftm
* Dependencies : none
*
* Autosar Version : 4.4.0
* Autosar Revision : ASR_REL_4_4_REV_0000
* Autosar Conf.Variant :
* SW Version : 4.0.2
* Build Version : S32_RTD_4_0_2_D2306_ASR_REL_4_4_REV_0000_20230630
*
* Copyright 2020-2023 NXP Semiconductors
*
* NXP Confidential. This software is owned or controlled by NXP and may only be
* used strictly in accordance with the applicable license terms. By expressly
* accepting such terms or by downloading, installing, activating and/or otherwise
* using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms. If you do not agree to be
* bound by the applicable license terms, then you may not retain, install,
* activate or otherwise use the software.
==================================================================================================*/

/**
*   @file           Gpt_PBcfg.c
*
*   @addtogroup     gpt Gpt Driver
*   @{
*/
#ifdef __cplusplus
extern "C"{
#endif

/*==================================================================================================
*                                         INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
===================================================================================================*/
#include "Gpt.h"
#include "Gpt_Cfg.h"
#include "Gpt_Ipw_PBcfg.h"
/*==================================================================================================
*                              SOURCE FILE VERSION INFORMATION
==================================================================================================*/
#define GPT_VENDOR_ID_PBCFG_C                      43
#define GPT_AR_RELEASE_MAJOR_VERSION_PBCFG_C       4
#define GPT_AR_RELEASE_MINOR_VERSION_PBCFG_C       4
#define GPT_AR_RELEASE_REVISION_VERSION_PBCFG_C    0
#define GPT_SW_MAJOR_VERSION_PBCFG_C               4
#define GPT_SW_MINOR_VERSION_PBCFG_C               0
#define GPT_SW_PATCH_VERSION_PBCFG_C               2

/*==================================================================================================
*                                     FILE VERSION CHECKS
==================================================================================================*/
#if (GPT_VENDOR_ID_PBCFG_C != GPT_VENDOR_ID_CFG)
    #error "Gpt_PBcfg.c and Gpt_Cfg.h have different vendor ids"
#endif
/* Check if the header files are of the same Autosar version */
#if ((GPT_AR_RELEASE_MAJOR_VERSION_PBCFG_C != GPT_AR_RELEASE_MAJOR_VERSION_CFG) || \
     (GPT_AR_RELEASE_MINOR_VERSION_PBCFG_C != GPT_AR_RELEASE_MINOR_VERSION_CFG) || \
     (GPT_AR_RELEASE_REVISION_VERSION_PBCFG_C != GPT_AR_RELEASE_REVISION_VERSION_CFG) \
    )
    #error "AutoSar Version Numbers of Gpt_PBcfg.c and Gpt_Cfg.h are different"
#endif
/* Check if the header files are of the same Software version */
#if ((GPT_SW_MAJOR_VERSION_PBCFG_C != GPT_SW_MAJOR_VERSION_CFG) || \
     (GPT_SW_MINOR_VERSION_PBCFG_C != GPT_SW_MINOR_VERSION_CFG) || \
     (GPT_SW_PATCH_VERSION_PBCFG_C != GPT_SW_PATCH_VERSION_CFG)    \
    )
    #error "Software Version Numbers of Gpt_PBcfg.c and Gpt_Cfg.h are different"
#endif

#if (GPT_VENDOR_ID_PBCFG_C != GPT_VENDOR_ID)
    #error "Gpt_PBcfg.c and Gpt.h have different vendor ids"
#endif
/* Check if the header files are of the same Autosar version */
#if ((GPT_AR_RELEASE_MAJOR_VERSION_PBCFG_C != GPT_AR_RELEASE_MAJOR_VERSION) || \
     (GPT_AR_RELEASE_MINOR_VERSION_PBCFG_C != GPT_AR_RELEASE_MINOR_VERSION) || \
     (GPT_AR_RELEASE_REVISION_VERSION_PBCFG_C != GPT_AR_RELEASE_REVISION_VERSION) \
    )
    #error "AutoSar Version Numbers of Gpt_PBcfg.c and Gpt.h are different"
#endif
/* Check if the header files are of the same Software version */
#if ((GPT_SW_MAJOR_VERSION_PBCFG_C != GPT_SW_MAJOR_VERSION) || \
     (GPT_SW_MINOR_VERSION_PBCFG_C != GPT_SW_MINOR_VERSION) || \
     (GPT_SW_PATCH_VERSION_PBCFG_C != GPT_SW_PATCH_VERSION)    \
    )
    #error "Software Version Numbers of Gpt_PBcfg.c and Gpt.h are different"
#endif

#if (GPT_VENDOR_ID_PBCFG_C != GPT_IPW_VENDOR_ID_PBCFG_H)
    #error "Gpt_PBcfg.c and Gpt_Ipw_PBcfg.h have different vendor ids"
#endif
/* Check if the header files are of the same Autosar version */
#if ((GPT_AR_RELEASE_MAJOR_VERSION_PBCFG_C != GPT_IPW_AR_RELEASE_MAJOR_VERSION_PBCFG_H) || \
     (GPT_AR_RELEASE_MINOR_VERSION_PBCFG_C != GPT_IPW_AR_RELEASE_MINOR_VERSION_PBCFG_H) || \
     (GPT_AR_RELEASE_REVISION_VERSION_PBCFG_C != GPT_IPW_AR_RELEASE_REVISION_VERSION_PBCFG_H) \
    )
    #error "AutoSar Version Numbers of Gpt_PBcfg.c and Gpt_Ipw_PBcfg.h are different"
#endif
/* Check if the header files are of the same Software version */
#if ((GPT_SW_MAJOR_VERSION_PBCFG_C != GPT_IPW_SW_MAJOR_VERSION_PBCFG_H) || \
     (GPT_SW_MINOR_VERSION_PBCFG_C != GPT_IPW_SW_MINOR_VERSION_PBCFG_H) || \
     (GPT_SW_PATCH_VERSION_PBCFG_C != GPT_IPW_SW_PATCH_VERSION_PBCFG_H)    \
    )
    #error "Software Version Numbers of Gpt_PBcfg.c and Gpt_Ipw_PBcfg.h. are different"
#endif

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
 *                                       GLOBAL VARIABLES
===================================================================================================*/
/**
*
* @brief The callback functions defined by the user to be called as channel notifications
*/
extern void Gpt_PitNotification(void);

/*==================================================================================================
*                                       GLOBAL CONSTANTS
==================================================================================================*/
#define GPT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Gpt_MemMap.h"
/* Number of channels per variant without partitions */
#define GPT_CONF_CHANNELS_PB 1U
#define GPT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Gpt_MemMap.h"


#define GPT_START_SEC_CONST_UNSPECIFIED
#include "Gpt_MemMap.h"
/*
*   @brief Translation LUT for Logical channel number to Partition Configuration indexed location
*/
static const uint8 u8GptChannelIdToIndexMap[GPT_NUM_CONFIG] = 
{
    0        /*Logical Channel GptChannelConfiguration_0*/
};
#define GPT_STOP_SEC_CONST_UNSPECIFIED
#include "Gpt_MemMap.h"


#define GPT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Gpt_MemMap.h"
/*
*  @brief    BOARD_InitPeripherals Configuration
*/
static const Gpt_ChannelConfigType Gpt_InitChannelPB[GPT_CONF_CHANNELS_PB]=
{
    {    /*GptChannelConfiguration_0 configuration data*/
        (boolean)FALSE,/* Wakeup capability */
        &Gpt_PitNotification, /* Channel notification */
#if ((GPT_WAKEUP_FUNCTIONALITY_API == STD_ON) && (GPT_REPORT_WAKEUP_SOURCE == STD_ON))
        (EcuM_WakeupSourceType)0U,/* Wakeup information */
#endif
        (Gpt_ValueType)(4294967295U),/* Maximum ticks value*/
        (GPT_CH_MODE_CONTINUOUS),/* Timer mode:continous/one-shot */
        &Gpt_Ipw_ChannelConfig_PB[0U]
    }
};

const Gpt_ConfigType Gpt_Config = 
{
    /** @brief Number of GPT channels (configured in tresos plugin builder)*/
    (Gpt_ChannelType)1U,
    /**@brief Pointer to the GPT channel configuration */
    &Gpt_InitChannelPB,
    /** @brief Number of GPT instances (configured in tresos plugin builder)*/
    1U,
    /** @brief Pointer to the GPT instance configuration */
    &Gpt_Ipw_HwInstanceConfig_PB,
#if(GPT_PREDEFTIMER_FUNCTIONALITY_API == STD_ON)
    /** @brief Pointer configuration of PredefTimer  */
    Gpt_pInitPredefTimerChannelPB,
#endif
    /** @brief Index of channel in each partition map table*/
    &u8GptChannelIdToIndexMap
};

#define GPT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Gpt_MemMap.h"

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
 *================================================================================================*/

/*==================================================================================================
 *                                       LOCAL FUNCTIONS
 *================================================================================================*/

/*==================================================================================================
 *                                   LOCAL FUNCTION PROTOTYPES
 *================================================================================================*/

#ifdef __cplusplus
}
#endif

