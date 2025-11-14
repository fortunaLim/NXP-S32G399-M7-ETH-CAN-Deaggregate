/*
*   (c) Copyright 2020 NXP
*   Modified for ISR optimization - RX processing moved to task
*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
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
#include "Gmac_Ip_Device_Registers.h"
#include "Gmac_Ip_Features.h"

#ifdef USING_OS_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif

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

// ==================== �߰�: RX ���� Ǯ ���� ====================
#define RX_BUFFER_POOL_SIZE    64    // RX ���� Ǯ ũ��
#define MAX_FRAME_SIZE         1536  // �ִ� �̴��� ������ ũ��

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
uint8 Gmac_0_MacAddr[6U] = {0x00,0x1b,0x19,0x00,0x00,0x00};


Gmac_Ip_VlanConfigType gmac_0_vlanConfig =
{
	/* .EnDoubleVlan = */ TRUE,
	/* .EnSvlan = */ FALSE,
	/* .OuterVlanIns = */ GMAC_VLAN_TAG_INSERTION,
	/* .InnerVlanIns = */ GMAC_VLAN_TAG_NO_CONTROL,
	/* .OuterVlanStrip = */ GMAC_VLAN_TAG_DO_NOT_STRIP,
	/* .InnerVlanStrip = */ GMAC_VLAN_TAG_DO_NOT_STRIP,
};

Gmac_Ip_VlanRxFilterType gmac_0_vlanRxFilterConfig = {
    /* .EnInnerVlanMatch = */ FALSE,
    /* .EnSvlanMatch = */ FALSE,
    /* .DisVlanTypeMatch = */ FALSE,
    /* .EnInverseMatch = */ FALSE,
    /* .En12bitMatch = */ TRUE,
};

// ==================== �߰�: RX ó���� ����ü �� ���� ====================
typedef struct {
    uint8 FrameData[MAX_FRAME_SIZE];
    uint16 FrameLength;
    uint8 CtrlIdx;
    uint8 FifoIdx;
} EthRxFrameBuffer_t;

// Bare metal �� ����
static EthRxFrameBuffer_t rxBufferPool[RX_BUFFER_POOL_SIZE];
static volatile uint16 rxBufferWriteIdx = 0;
static volatile uint16 rxBufferReadIdx = 0;
static volatile uint32 rxIsrCount = 0;
static volatile uint32 rxProcessedCount = 0;
static volatile uint32 rxDroppedCount = 0;

extern uint8 eth_rcv;

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
void Eth_Receive_SKKU(uint8 CtrlIdx, uint8 FifoIdx, Eth_RxStatusType *RxStatusPtr, uint8 **R_FrameData);
void Eth_Local_Memcpy(uint8 *Dst, const uint8 *Src, uint32 BytesNum);
void FlexCAN_init_SKKU(void);

// ���ο� �Լ���
static void ProcessReceivedFrame(const EthRxFrameBuffer_t *frameBuffer);
static void Eth_RxProcessPoll(void);

/*==================================================================================================
*                                       MAIN FUNCTION
==================================================================================================*/
extern const Mcu_ConfigType Mcu_Config;
extern const Port_ConfigType Port_Config;

void initializeSystem();
void runPTPMasterStateMachine();

Eth_RxStatusType Status;
uint8 *RxBuffer;

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

    /* Configure VLAN */
    Gmac_Ip_EnableVlan(EthConf_EthCtrlConfig_EthCtrlConfig_0, &gmac_0_vlanConfig);
    Gmac_Ip_SetTxOuterVlanTagForInsertion(EthConf_EthCtrlConfig_EthCtrlConfig_0,
                                           0U,
                                           GMAC_VLAN_TYPE_C_VLAN,
										   0x1309U
										   );


    for(;;)
    {
        // Bare metal: ���� ������� RX ó��
        Eth_RxProcessPoll();
    }
    return 0;
}

/*==================================================================================================
*                                   RX INTERRUPT CALLBACK (�淮ȭ)
==================================================================================================*/
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
uint8 sig_toggle = 0x01U;

void Gpt_PitNotification(void)
{
	Dio_WriteChannel(DioConf_DioChannel_DioChannel_0, sig_toggle);
	sig_toggle ^= 0xff;
}


// ==================== �ٽ� ����: ISR�� �ּ����� �۾��� ====================
void Eth_RxIrqCallback_SKKU(const uint8 CtrlIdx, const uint8 DMAChannel)
{
    Eth_RxStatusType status;
    uint8 *frameData = NULL;

    rxIsrCount++;

    // 1. ������ ������ ���� (�ϵ���� ���� ����)
    Eth_Receive_SKKU(CtrlIdx, 0U, &status, &frameData);

    if (status != ETH_NOT_RECEIVED && frameData != NULL)
    {

        // Bare metal: �� ���ۿ� ����
        uint16 nextWriteIdx = (rxBufferWriteIdx + 1) % RX_BUFFER_POOL_SIZE;

        if (nextWriteIdx != rxBufferReadIdx)  // ���� �����÷ο� üũ
        {
            rxBufferPool[rxBufferWriteIdx].CtrlIdx = CtrlIdx;
            rxBufferPool[rxBufferWriteIdx].FifoIdx = 0U;

            // �ּ����� ������ ����
            uint16 copyLen = MAX_FRAME_SIZE;
            Eth_Local_Memcpy(rxBufferPool[rxBufferWriteIdx].FrameData, frameData, copyLen);
            rxBufferPool[rxBufferWriteIdx].FrameLength = copyLen;

            rxBufferWriteIdx = nextWriteIdx;
        }
        else
        {
            rxDroppedCount++;  // ���� ���� ��
        }
    }

    // ISR ���� - ���⼭�� ������ ó�� �� ��!
}


/*==================================================================================================
*                              RX ó�� ���� (Bare Metal)
==================================================================================================*/
static void Eth_RxProcessPoll(void)
{
    // �� ���ۿ� �����Ͱ� ������ ó��
    while (rxBufferReadIdx != rxBufferWriteIdx)
    {
        rxProcessedCount++;

        // ���� ������ ó��
        ProcessReceivedFrame(&rxBufferPool[rxBufferReadIdx]);

        // �б� �ε��� ����
        rxBufferReadIdx = (rxBufferReadIdx + 1) % RX_BUFFER_POOL_SIZE;
    }
}


#if 1
// 상단 공통 위치에 추가 (보낸쪽과 맞추기)
#define ETHERTYPE_CONTAINER  0x0A0B  // 수신(집약) 프레임
#define ETHERTYPE_DEAGG      0x0A0C  // 재전송(분해) 프레임
#define GW_CONTAINER_VERSION 0x01
#define GW_CONTAINER_HDR_SIZE 2u      // Version(1) + Count(1)
#define GW_IPDU_HDR_SIZE      3u      // PduId(2) + Len(1)

static inline uint16 ReadBE16(const uint8 *p) { return (uint16)((p[0] << 8) | p[1]); }

// RX 처리 핵심: 컨테이너 → 개별 프레임
static void ProcessReceivedFrame(const EthRxFrameBuffer_t *frameBuffer)
{
    const uint8  *payload = frameBuffer->FrameData;   // ★ 위 1) 수정 덕분에 'payload' 시작이 들어있음
    uint16        payLen  = frameBuffer->FrameLength; // 드라이버가 준 payload 길이(ETH 헤더 제외)
    Eth_BufIdxType  txIdx;
    uint8         *txBuf;
    uint16         txBufLen;

    // 1) 컨테이너 최소 길이 점검
    if (payLen < GW_CONTAINER_HDR_SIZE) return;

    // (선택) EtherType 필터링을 payload 이전에서 하고 싶다면
    // Eth_Receive_SKKU()에서 EtherType도 함께 넘기도록 확장하면 좋음.
    // 여기서는 Version/Count로만 판단.
    uint8 version = payload[0];
    uint8 count   = payload[1];
    if (version != GW_CONTAINER_VERSION) return;

    uint16 pos = GW_CONTAINER_HDR_SIZE;

    // 2) Count 만큼 파싱 (오버런 방지 철저)
    for (uint8 i = 0; i < count; i++) {
        if (pos + GW_IPDU_HDR_SIZE > payLen) break; // 헤더 모자람
        uint16 pduId = ReadBE16(&payload[pos]);     // 2B
        uint8  len   = payload[pos + 2];            // 1B
        pos += GW_IPDU_HDR_SIZE;

        if (pos + len > payLen) break;              // Payload 모자람

        // 3) 송신 버퍼 획득 (우선 FIFO1로; 필요시 TC→FIFO 매핑 적용)
        txBufLen = 1518; // 최대 페이로드 가능치 요청
        if (Eth_ProvideTxBuffer(EthConf_EthCtrlConfig_EthCtrlConfig_0,
                                1U,       // Tx Ring 1(예: VLAN Priority 1)
                                &txIdx,
                                &txBuf,
                                &txBufLen) != E_OK) {
            // 버퍼 부족 → 이번 서브PDU는 드롭 (로그만)
            continue;
        }

        // 4) 새 프레임 페이로드 구성: [PduId(2B) | Len(1B) | Payload(len)]
        // 드라이버는 MAC/VLAN/Type를 알아서 붙이므로 여기엔 페이로드만 복사
        txBuf[0] = (uint8)(pduId >> 8);
        txBuf[1] = (uint8)(pduId & 0xFF);
        txBuf[2] = len;
        // 데이터 복사
        Eth_Local_Memcpy(&txBuf[3], &payload[pos], len);

        uint16 txLen = (uint16)(GW_IPDU_HDR_SIZE + len); // 우리가 보낼 페이로드 총길이

        // 5) 전송 (EtherType=0x0A0C). VLAN/우선순위는 컨트롤러 설정에 따름
        (void)Eth_Transmit(EthConf_EthCtrlConfig_EthCtrlConfig_0,
                           txIdx,
                           (Eth_FrameType)ETHERTYPE_DEAGG,
                           TRUE,            // TxConfirmation on
                           txLen,           // 페이로드 길이
                           Gmac_0_MacAddr   // Src MAC
                           );

        // 6) 다음 서브 PDU로
        pos += len;
    }
}
#endif


/*==================================================================================================
*                                   HELPER FUNCTIONS
==================================================================================================*/
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
    uint32 TempVar = BytesNum;
    while (TempVar > 0U)
    {
        TempVar--;
        Dst[TempVar] = Src[TempVar];
    }
}

void FlexCAN_init_SKKU(void)
{
    GMAC_Type *Base;
    uint8 dummyData[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
   // Base = Gmac_apxBases[0];

    FlexCAN_Ip_Init(INST_FLEXCAN_0, &FlexCAN_State0, &FlexCAN_Config0);
    FlexCAN_Ip_SetStartMode(INST_FLEXCAN_0);
}

#ifdef __cplusplus
}
#endif
