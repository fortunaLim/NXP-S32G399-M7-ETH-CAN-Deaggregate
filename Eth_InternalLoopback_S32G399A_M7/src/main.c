/*
*   (c) Copyright 2020 NXP
*
*   NXP Confidential. This software is owned or controlled by NXP and may only be used strictly
*   in accordance with the applicable license terms.  By expressly accepting
*   such terms or by downloading, installing, activating and/or otherwise using
*   the software, you are agreeing that you have read, and that you agree to
*   comply with and are bound by, such license terms.  If you do not agree to
*   be bound by the applicable license terms, then you may not retain,
*   install, activate or otherwise use the software.
*
*   This file contains sample code only. It is not part of the production code deliverables.
*/

#ifdef __cplusplus
extern "C" {
#endif


/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Mcu.h"
#include "Port.h"
#include "Eth.h"
#include "OsIf.h"
#include "Platform.h"
#include "string.h"
#include "check_example.h"
#include "Eth_Internal.h"
#include "Gmac_Ip_Types.h"
#include "Gmac_Ip.h"
#include "Eth_Ipw.h"
#include "Process_Queue.h"
#include "Dio.h"
#include "Dio_Cfg.h"
#include "Gpt.h"
#include "FlexCAN_Ip.h"

/*
#include "Siul2_Port_Ip.h"
#include "Siul2_Dio_Ip.h"
#include "Siul2_Port_Ip_Cfg.h"
*/
/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
#define ETH_FRAME_MACDST_OFFSET     (0U)
#define ETH_FRAME_MACSRC_OFFSET     (6U)
#define ETH_FRAME_ETHTYPE_OFFSET    (12U)
#define ETH_FRAME_PAYLOAD_OFFSET    (14U)

#define ETH_FRAME_MACDST_LENGTH     (6U)
#define ETH_FRAME_MACSRC_LENGTH     (6U)
#define ETH_FRAME_ETHTYPE_LENGTH    (2U)
#define ETH_FRAME_HEADER_LENGTH     (ETH_FRAME_MACDST_LENGTH + ETH_FRAME_MACSRC_LENGTH + ETH_FRAME_ETHTYPE_LENGTH)

#define ETH_BUFFER_INDEX_UNUSED     (0U)


/* PTP */
#define Typ_PTP                             0x88F7
#define IEEE1588v2                          0x02
#define Sync                                0x0
#define Delay_Req                           0x1
#define Pdelay_Req                          0x2
#define Pdelay_Resp                         0x3
#define Follow_Up                           0x8
#define Delay_Res                           0x9
#define Pdelay_Resp_Follow_Up               0xa
#define Announce                            0xb
#define Signaling                           0xc

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

/********************** Address For MAC  **********************/
//pdelay mac?   {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E};
//PTP MAC : (ra6m3) {0x00, 0x1B, 0x19, 0x01, 0x02, 0x03};
uint8                     PTP_Des_MAC[6]                    = {0x01, 0x1b, 0x19, 0x00, 0x00, 0x00};
//uint8_t                     PTP_Des_MAC[6]                    = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

const uint8                     Master_ClockID[8] = {0x01, 0x1b, 0x19, 0xFF, 0xFE, 0x01, 0x02, 0x03}; // PTP Local Clock ID
const uint8                     Slave_ClockID[8]  = {0x01, 0x1b, 0x19, 0xFF, 0xFE, 0x04, 0x05, 0x06}; // PTP Local Clock ID

const uint8                     Master_PortNum[2] = {0x00, 0x01}; // PTP Local PortNum
const uint8                     Slave_PortNum[2]  = {0x00, 0x02}; // PTP Local PortNum

uint8                     PTP_VLAN_TYPE[2] = {0x81, 0x00};

uint16_t                    Master_Sync_Sequence = 0;
uint16_t                    Master_Resp_Sequence = 0;
uint16_t                    Slave_Sequence = 0;

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/

typedef enum {
    INITIALIZING,
    MASTER_CLOCK_SELECTED,
    SYNCHRONIZING,
    FOLLOW_UP,
	DELAY_REQ,
	DELAY_RESP,
    ERROR_STATE
} PTP_MasterState;

PTP_MasterState currentState = INITIALIZING;


typedef struct _Ptp_PayloadTpye{

    /////////////////////////////////////////
    /** IEEE 1588v2 Layer (PTP version 2) **/
    /////////////////////////////////////////

    uint8                 MsgType         : 4;
    uint8                 trans_Spec      : 4;
    uint8                 versionPTP      : 4;
    uint8                 Reserved_a      : 4;
    uint8                 MsgLength[2];
    uint8                 DomainNumber;
    uint8                 Reserved_b;
    uint8                 flags[2];
    uint8                 correctField[8];
    uint8                 Reserved_c[4];
    uint8                 SrcClockID[8]; //???
    uint8                 SrcPortNum[2];
    uint8                 SequenceID[2];
    uint8                 ControlField;
    uint8                 LogMsgInterval;

    uint8                 orgTimeStamp_sec[6];
    uint8                 orgTimeStamp_nano[4];

    uint8			   	  Slave_ClockID[8];
	uint8 				  Slave_PortNum[2];
    uint8                 Padding[8];

    /////////////////////////////////////////
    /** Delay_Response (Extra Frame) **/
    /////////////////////////////////////////

    //uint8_t             req_Src_PortIdentity[ETH_HEAD_SIZE_6B];
    uint8                 req_Src_PortId[2];

}Ptp_PayloadTpye;


Ptp_PayloadTpye PTP_SYNC_Frame, PTP_RESP_Frame, PTP_Fallow_Frame;

QueueType PTP_Queue;

/**************************************************************************/
// Vlan
/**************************************************************************/
//Eth_Vlan_setting();
Gmac_Ip_VlanConfigType gmac_0_vlanConfig =
{
	/* .enDoubleVlan = */ TRUE,
	/* .enSvlan = */ FALSE,
	/* .outerVlanIns = */ GMAC_VLAN_TAG_INSERTION,//GMAC_VLAN_TAG_NO_CONTROL
	/* .innerVlanIns = */ GMAC_VLAN_TAG_INSERTION,
	/* .outerVlanStrip = */ GMAC_VLAN_TAG_DO_NOT_STRIP,
	/* .innerVlanStrip = */ GMAC_VLAN_TAG_DO_NOT_STRIP,
};

Gmac_Ip_VlanRxFilterType gmac_0_vlanRxFilterConfig = {
/* .enInnerVlanMatch = */ FALSE,
/* .enSvlanMatch = */ FALSE,
/* .disVlanTypeMatch = */ FALSE,
/* .enInverseMatch = */ FALSE,
/* .en12bitMatch = */ TRUE,
};

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
extern uint8 eth_rcv;
/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/


/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/
void Eth_Receive_SKKU(uint8 CtrlIdx,
                 uint8 FifoIdx,
                 Eth_RxStatusType *RxStatusPtr,
				 uint8 **R_FrameData
                );

void Ptp_MakeFrame(uint8 mode, // master, slave
                 uint8 msg_format, // sync, delay_req, pdelay_req...... etc
				 uint8 **BufPtr
                );

void runPTPMasterStateMachine(

				);

void initializeSystem();
void selectMasterClock();
void sendSynchronization();
void sendFollowUp();
void recvDealyReq();
void sendDelayResp();
void handleError();
void Total_time(uint64 * total_time ,uint32 sec, uint32 nano_sec);
void PTP_init();
void Print_IO();
void FlexCAN_init_SKKU(void);
void Eth_Local_Memcpy(uint8 *Dst, const uint8 *Src, uint32 BytesNum);
/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
void Eth_RxIrqCallback_SKKU(const uint8 CtrlIdx, const uint8 DMAChannel);

/*==================================================================================================
*
==================================================================================================*/
/**
* @brief        Transmit & receive in internal loopback mode
* @details      
*/
Eth_RxStatsType RxStatus;
Eth_BufIdxType BufferIndex, BufferIndex_RESP, BufferIndex_SYNC;
uint8 *PayloadBuffer;
uint8 *PayloadBuffer_RESP;
uint8 *RxBuffer;
uint16 PayloadLength = 46U;
uint8 Gmac_0_MacAddr[6U] = {0x01,0x1b,0x19,0x00,0x00,0x00};
//uint8 Gmac_0_MacAddr[6U] = {0x01,0x02,0x03,0x04,0x05,0x06};
Eth_RxStatusType Status;
boolean Pass = TRUE;
int i, a;
char *measage = "Hello Ethernet!!!";
Eth_TimeStampQualType TimeQualPtr, TimeQualCurrent;
Eth_TimeStampType TimeStampPtr, Time_Current, TimeStampCurrent2, TimeStampRESP, TimeStampSYNC;
volatile Eth_TimeStampType SW_Time;
sint64 Offset_array1[1000] = {0};
uint32 Offset_sec[1000] = {0};
uint32 Offset_nanosec[1000] = {0};
uint32 Offset_sec1[1000] = {0};
uint32 Offset_nanosec1[1000] = {0};
uint32 Offset_sec2[1000] = {0};
uint32 Offset_nanosec2[1000] = {0};
uint32 Offset_sec3[1000] = {0};
uint32 Offset_nanosec3[1000] = {0};
uint8 sig_toggle = 0x01U;
uint8 sig_toggle2 = 0x01U;

extern GMAC_Type * const Gmac_apxBases[FEATURE_GMAC_NUM_INSTANCES];
extern Gmac_Ip_StateType *Gmac_apxState[FEATURE_GMAC_NUM_INSTANCES];
extern Gmac_Ip_BufferDescriptorType *TxCurrentDesc[FEATURE_GMAC_NUM_CHANNELS]; /*!< The current available transmit buffer descriptor pointer array. */
extern Gmac_Ip_ChannelType * const Gmac_apxChBases[FEATURE_GMAC_NUM_INSTANCES][FEATURE_GMAC_NUM_CHANNELS];

uint32 status_monitor;
uint8 PTP_next_state;
uint32 Sec_Old=0;
uint32 send_count=0;
uint32 ttt=0;
sint64 PTP_Current;
volatile uint32 ptp_count_ref, ptp_count;
volatile uint8 PTP_Lock=0;

void Gpt_PitNotification(void)
{
	Dio_WriteChannel(DioConf_DioChannel_DioChannel_0, sig_toggle);
	sig_toggle ^= 0xff;
}

int main(void) 
{
	Eth_TimeStampType Time_Current_in;

  	OsIf_Init(NULL_PTR);
    
    /* Initialize all pins using the Port driver */
    Port_Init(NULL_PTR);
 	//Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);

    /* Initialize the Mcu driver */
    Mcu_Init(NULL_PTR);

    /* Initialize the clock tree and apply PLL as system clock */
    Mcu_InitClock(McuClockSettingConfig_0);
    
    while ( MCU_PLL_LOCKED != Mcu_GetPllStatus() )
    {
        /* Busy wait until the System PLL is locked */
    }

    Mcu_DistributePllClock();
    
    Mcu_SetMode(McuModeSettingConf_0);

    Platform_Init(NULL_PTR);

    Eth_Init(NULL_PTR);

    FlexCAN_init_SKKU();

    Eth_SetControllerMode(EthConf_EthCtrlConfig_EthCtrlConfig_0, ETH_MODE_ACTIVE);

    //Gmac_Ip_EnableVlan(EthConf_EthCtrlConfig_EthCtrlConfig_0, &gmac_0_vlanConfig);
    //initializeSystem();


/***********************************
   Use GPT when Software Mode
 ***********************************/

    GMAC_Type *Base;
    Gmac_Ip_BufferDescriptorType *Bd;
    Base = Gmac_apxBases[0];
   // Bd = Gmac_apxChBases[0][0];//Gmac_apxState[0]->TxCurrentDesc[0];
    Bd = Gmac_apxState[0]->TxCurrentDesc[0];

/***********************************
   Offload SYNC trig on
 ***********************************/


/***********************************
   Main Loop
 ***********************************/

    for(;;)
    {


    }
    return 0;
}


/***********************************
   Software Mode
 ***********************************/
// Gmac_Ip_PBcfg.c -> GMAC_0_aRxRingConfigPB -> Eth_RxIrqCallback_SKKU();
Flexcan_Ip_MsgBuffType rxFifoData, txData, txData1, txData2, txData3;

Flexcan_Ip_DataInfoType tx_info = {
		.msg_id_type = FLEXCAN_MSG_ID_STD,
		.data_length = 8u,
		.fd_enable = FALSE,
		.fd_padding = FALSE,
		.enable_brs = FALSE,
		.is_polling = FALSE,
		.is_remote = FALSE
	};

volatile uint32 counter;
uint8 TX_MB_IDX;


void Eth_RxIrqCallback_SKKU(const uint8 CtrlIdx, const uint8 DMAChannel)
{
	//uint8 ii=0;
	uint8 dummyData_E2C[8] = {0x0};
	uint32 index_temp;
	uint16 CAN_i,CAN_j ;
	uint16 CAN_MSG_ID = 0;
	uint8 CAM_MSG_LEN = 0;
	uint8 PDU_num_Tx = 0;
	uint32 CAN_msg_index = 18;
	uint8 Gmac_0_MacAddr[6U] = {0x01,0x1b,0x19,0x00,0x00,0x00};
	Eth_BufIdxType BufferIndex2, BufferIndex_RX;
	uint8 *PayloadBuffer2;

	Eth_Receive_SKKU(EthConf_EthCtrlConfig_EthCtrlConfig_0, 0U, &Status, &RxBuffer);

	CAN_msg_index = 12;
	PDU_num_Tx = RxBuffer[CAN_msg_index];
	CAN_msg_index++;

	for(CAN_i=0;CAN_i<PDU_num_Tx;CAN_i++)
	{
		Eth_ProvideTxBuffer(EthConf_EthCtrlConfig_EthCtrlConfig_0, 1U, &BufferIndex2, &PayloadBuffer2, 32U);

		CAN_MSG_ID  = (RxBuffer[CAN_msg_index]>>4) | RxBuffer[CAN_msg_index+1]<<4;
		CAM_MSG_LEN = RxBuffer[CAN_msg_index] & 0xf;
		RxBuffer[CAN_msg_index] = CAN_MSG_ID;
		CAN_msg_index = CAN_msg_index + 2;

		index_temp = CAN_msg_index;

		for(CAN_msg_index; CAN_msg_index<index_temp+CAM_MSG_LEN; CAN_msg_index++)
		{
			dummyData_E2C[CAN_msg_index-index_temp] = RxBuffer[CAN_msg_index];
		}

		//memcpy(&dummyData_E2C[CAN_msg_index], &bd.Data[CAN_msg_index], CAM_MSG_LEN);
		//CAN_msg_index = CAN_msg_index+CAM_MSG_LEN;
		tx_info.data_length = CAM_MSG_LEN;

		//FlexCAN_Ip_SendBlocking(0U, TX_MB_IDX, &tx_info, CAN_MSG_ID, (uint8 *)&dummyData_E2C, 10000);
		//FlexCAN_Ip_AbortTransfer(0U, TX_MB_IDX);
		//TX_MB_IDX = 8+(TX_MB_IDX+1)%8;
		PayloadBuffer2[0] = CAN_MSG_ID;
		Eth_Local_Memcpy(&PayloadBuffer2[1], &RxBuffer[index_temp], 2+CAM_MSG_LEN);
		Eth_Transmit(EthConf_EthCtrlConfig_EthCtrlConfig_0, BufferIndex2, (Eth_FrameType)0x0A0CU, FALSE, 55U, Gmac_0_MacAddr);
	}
}

/*================================================================================================*/
void Eth_Receive_SKKU(uint8 CtrlIdx,
                 uint8 FifoIdx,
                 Eth_RxStatusType *RxStatusPtr,
				 uint8 **R_FrameData
                )
{
    uint8 *FrameData;
    uint16 FrameLength;
    boolean FrameHasError;

    Eth_FrameType FrameType;
    boolean IsBroadcast;
    const uint8 *MacSrcAddr;
    const Eth_DataType *Payload;
    uint16 PayloadLength;

#if (STD_ON == ETH_SWT_MANAGEMENT_SUPPORT_API)
    boolean IsMgmtFrameOnly;
    uint8 *DataMgmt;
    const Eth_CtrlCfgType *Eth_Controller;
#endif

#if (STD_ON == ETH_SWT_MANAGEMENT_SUPPORT_API) || (STD_ON == ETH_DEV_ERROR_DETECT)
    uint32 CoreId = Eth_GetCoreID();
#endif

#if STD_ON == ETH_DEV_ERROR_DETECT
    if (0U == Eth_Local_CheckEthHLDEntry(CoreId, CtrlIdx, ETH_SID_RECEIVE))
    {
        if(ETH_MODE_ACTIVE != Eth_Ipw_GetControllerMode(CtrlIdx))
        {
            (void)Det_ReportError(ETH_MODULE_ID, ETH_DRIVER_INSTANCE, ETH_SID_RECEIVE, ETH_E_INV_MODE);
        }
        else
        {
#endif /* ETH_DEV_ERROR_DETECT  */
            *RxStatusPtr = Eth_Ipw_ReceiveFrame(CtrlIdx, FifoIdx, &FrameData, &FrameLength, &FrameHasError);

            if ((ETH_NOT_RECEIVED != *RxStatusPtr) && ((boolean)FALSE == FrameHasError))
            {

            #if (STD_ON == ETH_SWT_MANAGEMENT_SUPPORT_API)
                DataMgmt = &FrameData[ETH_FRAME_ETHTYPE_OFFSET];
                Eth_Controller = Eth_apxInternalCfg[CoreId]->Eth_apCtrlConfig[CtrlIdx];
                /* There shouldn't be a buffer index concept for RX side */
                (void)Eth_Controller->Eth_EthSwtDriverFunctionList.RxProcessFrameFunction(CtrlIdx, ETH_BUFFER_INDEX_UNUSED, &DataMgmt, &FrameLength, &IsMgmtFrameOnly);
                if ((boolean)FALSE == IsMgmtFrameOnly)
                {
            #endif
                    /* Get Frame Type */
                    FrameType  = FrameData[ETH_FRAME_ETHTYPE_OFFSET + 1U];
                    FrameType |= ((uint16)FrameData[ETH_FRAME_ETHTYPE_OFFSET] << 8U);

                    /* Check Broadcast */
                    //IsBroadcast = Eth_Local_IsBroadcastFrame(&FrameData[ETH_FRAME_MACDST_OFFSET]);

                    /* Get MAC Source Address */
                    MacSrcAddr = &FrameData[ETH_FRAME_MACSRC_OFFSET];

                    /* Get Payload */
                    Payload = &FrameData[ETH_FRAME_PAYLOAD_OFFSET];

                    /* Get Payload Length */
                    /* We assume CRC and padding have been stripped so that FrameLength = DST + SRC + (VLAN_TAG) + ETHTYPE + PAYLOAD */
                    PayloadLength = FrameLength - ETH_FRAME_HEADER_LENGTH;

                    *R_FrameData = &FrameData[ETH_FRAME_MACSRC_OFFSET];
                    //memcpy(R_FrameData,FrameData,(14+PayloadLength)*sizeof(char));

                    //EthIf_RxIndication(CtrlIdx, FrameType, IsBroadcast, MacSrcAddr, Payload, PayloadLength);
            #if (STD_ON == ETH_SWT_MANAGEMENT_SUPPORT_API)
                }

                (void)Eth_Controller->Eth_EthSwtDriverFunctionList.RxFinishedIndicationFunction(CtrlIdx, ETH_BUFFER_INDEX_UNUSED);
            #endif
            }
            if (ETH_NOT_RECEIVED != *RxStatusPtr)
            {
                Eth_Ipw_ReleaseResources(CtrlIdx, FifoIdx);
            }

#if STD_ON == ETH_DEV_ERROR_DETECT
        }
    }
#endif /* ETH_DEV_ERROR_DETECT  */
}

void Eth_Local_Memcpy(uint8 *Dst, const uint8 *Src, uint32 BytesNum)
{
    uint8 TempVar = BytesNum;

    /* Start copy data*/
    while (TempVar > 0U)
    {
        TempVar--;
        Dst[TempVar] = Src[TempVar];
    }
}

void FlexCAN_init_SKKU(void)
{
	GMAC_Type *Base;

	uint8 dummyData[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	Base = Gmac_apxBases[0];
    /*NVIC_SetPriority(CAN0_ORED_0_7_MB_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(CAN0_ORED_0_7_MB_IRQn);*/

	FlexCAN_Ip_Init(INST_FLEXCAN_0, &FlexCAN_State0, &FlexCAN_Config0);

	//FlexCAN_Ip_ConfigEnhancedRxFifo_Privileged(INST_FLEXCAN_0, &CAN0_EnhanceFIFO_IdFilterTable[0]);
	//FlexCAN_Ip_ConfigRxFifo(0, FLEXCAN_RX_FIFO_ID_FORMAT_A, &CAN0_FIFO_IdFilterTable[2]);

	//FlexCAN_Ip_SetRxMaskType_Privileged(INST_FLEXCAN_0,FLEXCAN_RX_MASK_INDIVIDUAL);
	//FlexCAN_Ip_SetRxIndividualMask_Privileged(INST_FLEXCAN_0,1,0x1FFFFFFF);
	//FlexCAN_Ip_SetRxIndividualMask_Privileged(INST_FLEXCAN_0,2,0x1FFFFFFF);

    //FlexCAN_Ip_ConfigRxMb(INST_FLEXCAN_0, RX_MB_IDX, &rx_info, 0x1);
    //FlexCAN_Ip_ConfigRxMb(INST_FLEXCAN_0, 2, &rx_info, 0x2);

	FlexCAN_Ip_SetStartMode(INST_FLEXCAN_0);

    //FlexCAN_Ip_Receive(INST_FLEXCAN_0, RX_MB_IDX, &rxData, false);
    //FlexCAN_Ip_Receive(INST_FLEXCAN_0, 2, &rxData, false);
    //FlexCAN_Ip_RxFifo(INST_FLEXCAN_0, &rxFifoData);

    //FlexCAN_Ip_SendBlocking(0, 8, &tx_info, 20U, (uint8 *)&dummyData, 10000);

}




#ifdef __cplusplus
}
#endif

/** @} */
