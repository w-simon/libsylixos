/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ahci.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 01 月 04 日
**
** 描        述: AHCI 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "linux/compat.h"
#include "ahci.h"
#include "ahciLib.h"
#include "ahciPort.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciCtrl.h"
#include "ahciPm.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT32   _GuiAhciBitMask[AHCI_DRIVE_MAX] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
};
static INT  _GiAhciConfigType[AHCI_DRIVE_MAX] = {
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL,
    AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL, AHCI_MODE_ALL
};
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static PVOID            __ahciMonitorThread(PVOID pvArg);
static irqreturn_t      __ahciIsr(PVOID pvArg, ULONG ulVector);
static INT              __ahciDiskCtrlInit(AHCI_CTRL_HANDLE hCtrl, INT iDrive);
/*********************************************************************************************************
** 函数名称: __ahciCmdWaitForResource
** 功能描述: 等待资源
** 输　入  : hDrive     驱动器句柄
**           bQueued    是否使能队列
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __ahciCmdWaitForResource (AHCI_DRIVE_HANDLE  hDrive, BOOL  bQueued)
{
             INTREG iregInterLevel;
    REGISTER INT    i = 0;                                              /* 循环因子                     */

    if (hDrive->AHCIDRIVE_bNcq == LW_FALSE) {                           /* 非 NCQ 模式                  */
        API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
    
    } else {                                                            /* NCQ 模式                     */
        if (bQueued) {                                                  /* 队列模式                     */
            API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem, LW_OPTION_WAIT_INFINITE);
                                                                        /* 等待控制权                   */
            LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel);
            if (hDrive->AHCIDRIVE_bQueued == LW_FALSE) {                /* 非队列模式                   */
                hDrive->AHCIDRIVE_bQueued = LW_TRUE;                    /* 队列模式                     */
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                                                                        /* 是否控制权                   */
                                                                        /* 释放队列槽控制权             */
                for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth - 1; i++) {
                    API_SemaphoreCPost(hDrive->AHCIDRIVE_hQueueSlotCSem);
                }
            } else {                                                    /* 队列模式                     */
                LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);
                                                                        /* 是否控制权                   */
            }
        
        } else {                                                        /* 非队列模式                   */
            API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
            
            if (hDrive->AHCIDRIVE_bQueued == LW_TRUE) {                 /* 队列模式                     */
                for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth; i++) {  /* 释放队列资源                 */
                    API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem, LW_OPTION_WAIT_INFINITE);
                }
                hDrive->AHCIDRIVE_bQueued = LW_FALSE;                   /* 标记为非队列模式             */
            
            } else {                                                    /* 非队列模式                   */
                API_SemaphoreCPend(hDrive->AHCIDRIVE_hQueueSlotCSem, LW_OPTION_WAIT_INFINITE);
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: __ahciCmdReleaseResource
** 功能描述: 释放资源
** 输　入  : hDrive     驱动器句柄
**           bQueued    是否使能队列
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __ahciCmdReleaseResource (AHCI_DRIVE_HANDLE  hDrive, BOOL  bQueued)
{
    if (hDrive->AHCIDRIVE_bNcq == LW_FALSE) {                           /* 非队列模式                   */
        API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
    
    } else {                                                            /* 队列模式                     */
        API_SemaphoreCPost(hDrive->AHCIDRIVE_hQueueSlotCSem);
        
        if (bQueued == LW_FALSE) {                                      /* 非队列模式                   */
            API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __ahciPrdtSetup
** 功能描述: 设置 Physical Region Descriptor Table (PRDT)
** 输　入  : pcDataBuf      数据缓冲区
**           ulLen          数据长度
**           hPrdtHandle    PRDT 控制块句柄
** 输　出  : 数据字节长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static size_t  __ahciPrdtSetup (UINT8 *pcDataBuf, ULONG  ulLen, AHCI_PRDT_HANDLE  hPrdtHandle)
{
    size_t      stPrdtCount = 0;                                        /* PRD 计数                     */
    ULONG       ulByteCount = 0;                                        /* 数据长度计数                 */
    ULONG       ulSize      = 0;                                        /* 数据长度                     */
    PVOID       pvAddr      = LW_NULL;                                  /* 数据缓冲区                   */

    AHCI_CMD_LOG(AHCI_LOG_PRT, "buff 0x%x len %ld.", pcDataBuf, ulLen);

    pvAddr = (PVOID)pcDataBuf;
    if ((ULONG)pvAddr & 1) {                                            /* 地址对齐错误                 */
        AHCI_CMD_LOG(AHCI_LOG_ERR, "dma buffer not word aligned.", 0);
        return  ((size_t)PX_ERROR);
    }

    stPrdtCount = 0;
    ulSize = ulLen;
    while (ulSize) {                                                    /* 构造 PRDT                    */
        if (++stPrdtCount > AHCI_PRDT_MAX) {                            /* PRDT 块数量超限              */
            AHCI_CMD_LOG(AHCI_LOG_ERR, "dma table too small.", 0);
            return  ((size_t)PX_ERROR);
        
        } else {                                                        /* 有可用的 PRDT 块             */
            if (ulSize > AHCI_PRDT_BYTE_MAX) {                          /* 数据大小超限                 */
                ulByteCount = AHCI_PRDT_BYTE_MAX;
            } else {                                                    /* 数据大小未超限               */
                ulByteCount = ulSize;
            }

            /*
             *  构造 PRDT 地址信息
             */
            hPrdtHandle->AHCIPRDT_uiDataBaseAddrLow  = AHCI_SWAP(AHCI_ADDR_LOW32(pvAddr));
            hPrdtHandle->AHCIPRDT_uiDataBaseAddrHigh = AHCI_SWAP(AHCI_ADDR_HIGH32(pvAddr));

            /*
             *  更新地址与大小等信息
             */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "table addr %p byte count %ld.", pvAddr, ulByteCount);
            pvAddr  = (PVOID)((ULONG)pvAddr + ulByteCount);
            ulSize -= ulByteCount;

            ulByteCount--;
            if (ulSize == 0) {
                ulByteCount |= AHCI_PRDT_I;
            }

            hPrdtHandle->AHCIPRDT_uiInterruptDataByteCount = AHCI_SWAP(ulByteCount);
            AHCI_CMD_LOG(AHCI_LOG_PRT, "table count : 0x%x.", ulByteCount);
            hPrdtHandle++;
        }
    }

    API_CacheDmaFlush(hPrdtHandle, stPrdtCount * sizeof(AHCI_PRDT_CB)); /* 回写 PRDT 参数信息           */

    return  (stPrdtCount);
}
/*********************************************************************************************************
** 函数名称: __ahciDiskCommandSend
** 功能描述: 向磁盘驱动器发送命令
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器号
**           hCmd       命令句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDiskCommandSend (AHCI_CTRL_HANDLE  hCtrl, UINT  uiDrive, AHCI_CMD_HANDLE  hCmd)
{
    ULONG                   ulRet            = PX_ERROR;                /* 操作结果                     */
    AHCI_DRIVE_HANDLE       hDrive           = LW_NULL;                 /* 驱动器句柄                   */
    AHCI_PRDT_HANDLE        hPrdt            = LW_NULL;                 /* PRDT 句柄                    */
    UINT8                  *pucCommandFis    = LW_NULL;                 /* 命令结构缓冲                 */
    UINT8                  *pucPktCmd        = LW_NULL;                 /* 命令缓冲                     */
    AHCI_CMD_LIST_HANDLE    hCommandList     = LW_NULL;                 /* 命令序列句柄                 */
    UINT32                  uiFlagsPrdLength = 0;                       /* PRDT 标志                    */
    size_t                  stPrdtCount      = 0;                       /* PRDT 计数信息                */
    INTREG                  iregInterLevel   = 0;                       /* 中断寄存器                   */
    INT                     iTag             = 0;                       /* 标记信息                     */
    ULONG                   ulWait           = 0;                       /* 超时参数                     */
    AHCI_MSG_CB             tCtrlMsg;                                   /* 消息控制块                   */
    AHCI_MSG_HANDLE         hCtrlMsg         = LW_NULL;                 /* 消息句柄                     */
    UINT32                  uiTagBit         = 0;                       /* 标记位信息                   */
    UINT32                  uiTemp           = 0;                       /* 临时变量                     */
    BOOL                    bQueued          = LW_FALSE;                /* 队列模式                     */

    UINT16                  usCylinder       = 0;                       /* 柱面                         */
    UINT16                  usHead           = 0;                       /* 磁头                         */
    UINT16                  usSector         = 0;                       /* 扇区                         */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* 获取驱动器句柄               */
    hCtrlMsg = &tCtrlMsg;
    bQueued = (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ);               /* 是否使能队列模式             */
    uiTemp = hCmd->AHCICMD_iFlags & (AHCI_CMD_FLAG_SRST_ON | AHCI_CMD_FLAG_SRST_OFF);
    if (uiTemp == 0) {                                                  /* 非控制命令                   */
        __ahciCmdWaitForResource(hDrive, bQueued);                      /* 等待控制权                   */
    }

    if (bQueued) {                                                      /* 队列模式使能                 */
        API_SemaphoreMPend(hDrive->AHCIDRIVE_hTagMuteSem, LW_OPTION_WAIT_INFINITE);

        do {                                                            /* 查找有效标志位               */
            iTag = hDrive->AHCIDRIVE_iNextTag;
            hDrive->AHCIDRIVE_iNextTag =(hDrive->AHCIDRIVE_iNextTag + 1) % hDrive->AHCIDRIVE_uiQueueDepth;
        } while (hDrive->AHCIDRIVE_uiCmdStarted & (_GuiAhciBitMask[iTag]));

        uiTagBit = _GuiAhciBitMask[iTag];                               /* 获取标志位信息               */
        API_SemaphoreMPost(hDrive->AHCIDRIVE_hTagMuteSem);              /* 释放控制权                   */
    
    } else {                                                            /* 非队列模式                   */
        iTag     = 0;                                                   /* 标记索引                     */
        uiTagBit = 1;                                                   /* 标记位信息                   */
    }

    /*
     *  获取 PRDT 控制块与命令序列控制块
     */
    hPrdt = &hDrive->AHCIDRIVE_hCmdTable[iTag].AHCICMDTABL_tPrdt[0];
    pucCommandFis = hDrive->AHCIDRIVE_hCmdTable[iTag].AHCICMDTABL_ucCommandFis;
    pucPktCmd = hDrive->AHCIDRIVE_hCmdTable[iTag].AHCICMDTABL_ucAtapiCommand;
    hCommandList = &hDrive->AHCIDRIVE_hCmdList[iTag];

    pucCommandFis[0] = 0x27;
    if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_SRST_ON) {                 /* 复位控制器                   */
        AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd srst on.", 0);
        lib_bzero(&pucCommandFis[1], 19);
        pucCommandFis[15] = AHCI_CTL_4BIT | AHCI_CTL_RST;
        hCommandList->AHCICMDLIST_uiPrdtFlags =  AHCI_SWAP(AHCI_CMD_LIST_C | AHCI_CMD_LIST_R | 5);
        API_CacheDmaFlush(pucCommandFis, 20);
        AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
        return  (ERROR_NONE);
    
    } else if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_SRST_OFF) {         /* 复位控制器完成               */
        AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd srst off.", 0);
        lib_bzero(&pucCommandFis[1], 19);
        pucCommandFis[15] = AHCI_CTL_4BIT;
        hCommandList->AHCICMDLIST_uiPrdtFlags =  AHCI_SWAP(5);
        API_CacheDmaFlush(pucCommandFis, 20);
        AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
        return  (ERROR_NONE);
    
    } else {                                                            /* 其它命令                     */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI 工作模式               */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd flag atapi.", 0);
            pucCommandFis[1] = 0x80;
            pucCommandFis[2] = AHCI_PI_CMD_PKTCMD;
            pucCommandFis[3] = hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiFeature;
            pucCommandFis[4] = 0;
            pucCommandFis[5] = (UINT8)hCmd->AHCICMD_ulDataLen;
            pucCommandFis[6] = (UINT8)(hCmd->AHCICMD_ulDataLen >> 8);
            pucCommandFis[7] = AHCI_SDH_LBA;
            lib_bzero(&pucCommandFis[8], 12);
            lib_memcpy(pucPktCmd, hCmd->AHCI_CMD_ATAPI.AHCICMDATAPI_ucAtapiCmdPkt,AHCI_ATAPI_CMD_LEN_MAX);
        
        } else {                                                        /* ATA 工作模式                 */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "cmd flag ata.", 0);
            pucCommandFis[1] = 0x80;
            pucCommandFis[2] = hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand;
            
            if ((hDrive->AHCIDRIVE_bLba == LW_TRUE) ||
                (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NON_SEC_DATA)) {  /* LBA 模式或无扩展数据         */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "lba true flag non sec data.", 0);
                pucCommandFis[4] = (UINT8)hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba;
                pucCommandFis[5] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 8);
                pucCommandFis[6] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 16);
                pucCommandFis[7] = AHCI_SDH_LBA;
                
                if (hDrive->AHCIDRIVE_bLba48 == LW_TRUE) {              /* LBA 48 模式                  */
                    pucCommandFis[ 8] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 24);
                    pucCommandFis[ 9] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 32);
                    pucCommandFis[10] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 40);
                
                } else {                                                /* 非 LBA 48 模式               */
                    uiTemp = (UINT8)(pucCommandFis[7] |
                                     ((hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba >> 24) & 0x0F));
                    pucCommandFis[ 7] = (UINT8)uiTemp;
                    pucCommandFis[ 8] = 0;
                    pucCommandFis[ 9] = 0;
                    pucCommandFis[10] = 0;
                }
            
            } else {                                                    /* 非 LBA 模式                  */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "lba false flag sec data.", 0);
                usCylinder = (UINT16)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba /
                                      (hDrive->AHCIDRIVE_uiSector * hDrive->AHCIDRIVE_uiHead));
                usSector = (UINT16)(hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba %
                                    (hDrive->AHCIDRIVE_uiSector * hDrive->AHCIDRIVE_uiHead));
                usHead = usSector / hDrive->AHCIDRIVE_uiSector;
                usSector = usSector % hDrive->AHCIDRIVE_uiSector + 1;

                pucCommandFis[ 4] = (UINT8)usSector;
                pucCommandFis[ 5] = (UINT8)(usCylinder >> 8);
                pucCommandFis[ 6] = (UINT8)usCylinder;
                pucCommandFis[ 7] = (UINT8)(AHCI_SDH_IBM | (usHead & 0x0F));
                pucCommandFis[ 8] = 0;
                pucCommandFis[ 9] = 0;
                pucCommandFis[10] = 0;
            }

            if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ) {             /* 使能 NCQ                     */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "flag ncq.", 0);
                pucCommandFis[ 3] = (UINT8)hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount;
                pucCommandFis[11] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount >> 8);
                pucCommandFis[12] = (UINT8)(iTag << 3);
                pucCommandFis[13] = 0;
                pucCommandFis[ 7] = 0x40;
            
            } else {                                                    /* 禁能 NCQ                     */
                AHCI_CMD_LOG(AHCI_LOG_PRT, "flag no ncq.", 0);
                pucCommandFis[ 3] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature);
                pucCommandFis[11] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature >> 8);
                pucCommandFis[12] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount);
                pucCommandFis[13] = (UINT8)(hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount >> 8);
            }
            
            pucCommandFis[14] = 0;
            pucCommandFis[15] = AHCI_CTL_4BIT;
            pucCommandFis[16] = 0;
            pucCommandFis[17] = 0;
            pucCommandFis[18] = 0;
            pucCommandFis[19] = 0;
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x",
                     pucCommandFis[0], pucCommandFis[1],pucCommandFis[2], pucCommandFis[3],
                     pucCommandFis[4], pucCommandFis[5],pucCommandFis[6], pucCommandFis[7]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x",
                     pucCommandFis[ 8], pucCommandFis[ 9], pucCommandFis[10], pucCommandFis[11],
                     pucCommandFis[12], pucCommandFis[13], pucCommandFis[14], pucCommandFis[15]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x",
                     pucCommandFis[16], pucCommandFis[17], pucCommandFis[18], pucCommandFis[19],
                     pucCommandFis[20], pucCommandFis[21], pucCommandFis[22], pucCommandFis[23]);
        AHCI_CMD_LOG(AHCI_LOG_PRT,"fis: %02x %02x %02x %02x %02x %02x %02x %02x",
                     pucCommandFis[24], pucCommandFis[25], pucCommandFis[26], pucCommandFis[27],
                     pucCommandFis[28], pucCommandFis[29], pucCommandFis[30], pucCommandFis[31]);

        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_NONE) {           /* 无效方向                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "data dir none.", 0);
            uiFlagsPrdLength = 5;
        
        } else {                                                        /* 有效方向                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT,
                         "data dir %s.", hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_IN ? "read" : "write");
                                                                        /* 构造 PRDT                    */
            stPrdtCount = __ahciPrdtSetup(hCmd->AHCICMD_pucDataBuf, hCmd->AHCICMD_ulDataLen, hPrdt);
            if (stPrdtCount == (size_t)PX_ERROR) {                      /* 构造失败                     */
                __ahciCmdReleaseResource(hDrive, bQueued);              /* 是否控制权                   */
                return  (PX_ERROR);
            }
                                                                        /* 设置标志                     */
            uiFlagsPrdLength = (UINT32)(stPrdtCount << AHCI_CMD_LIST_PRDTL_SHFT) | 5;
            if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_OUT) {        /* 输出模式                     */
                uiFlagsPrdLength |= AHCI_CMD_LIST_W;                    /* 写操作标志                   */
            }
            if ((hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ) == 0) {      /* 非 NCQ 模式                  */
                uiFlagsPrdLength |= AHCI_CMD_LIST_P;                    /* 禁能 NCQ                     */
            }
        }
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI 模式                   */
            uiFlagsPrdLength |= (AHCI_CMD_LIST_A | AHCI_CMD_LIST_P);
        }
        hCommandList->AHCICMDLIST_uiPrdtFlags = AHCI_SWAP(uiFlagsPrdLength);
        hCommandList->AHCICMDLIST_uiByteCount = 0;

        API_CacheDmaFlush(hCommandList, sizeof(hCommandList));          /* 回写命令列表信息             */
        API_CacheDmaFlush(pucCommandFis, 20);                           /* 回写命令信息                 */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_ATAPI) {               /* ATAPI 模式                   */
            API_CacheDmaFlush(pucPktCmd, AHCI_ATAPI_CMD_LEN_MAX);       /* 回写命令信息                 */
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "flag prdt length 0x%08x.", hCommandList->AHCICMDLIST_uiPrdtFlags);
        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_OUT) {            /* 输出模式                     */
                                                                        /* 回写数据区                   */
            API_CacheDmaFlush(hCmd->AHCICMD_pucDataBuf, (size_t)hCmd->AHCICMD_ulDataLen);
        }

        LW_SPIN_LOCK_QUICK(&hDrive->AHCIDRIVE_slLock, &iregInterLevel); /* 等待控制权                   */
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_NCQ) {                 /* NCQ 模式                     */
            AHCI_PORT_WRITE(hDrive, AHCI_PxSACT, uiTagBit);
            AHCI_PORT_READ(hDrive, AHCI_PxSACT);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
            AHCI_PORT_READ(hDrive, AHCI_PxCI);
            hDrive->AHCIDRIVE_uiCmdStarted |= uiTagBit;
        
        } else {                                                        /* 非 NCQ 模式                  */
            AHCI_PORT_WRITE(hDrive, AHCI_PxCI, uiTagBit);
            AHCI_PORT_READ(hDrive, AHCI_PxCI);
            hDrive->AHCIDRIVE_uiCmdStarted |= uiTagBit;
        }
        LW_SPIN_UNLOCK_QUICK(&hDrive->AHCIDRIVE_slLock, iregInterLevel);/* 是否控制权                   */

        AHCI_CMD_LOG(AHCI_LOG_PRT, "start command.", 0);
        if (hCmd->AHCICMD_iFlags & AHCI_CMD_FLAG_WAIT_SPINUP) {         /* 更新超时时间参数             */
            ulWait = API_TimeGetFrequency() * 20;
        } else {                                                        /* 使用初始超时时间参数         */
            ulWait = hCtrl->AHCICTRL_ulSemTimeout;
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "sem take tag 0x%08x wait 0x%08x.", iTag, ulWait);
                                                                        /* 等待操作结果                 */
        ulRet = API_SemaphoreBPend(hDrive->AHCIDRIVE_hSyncBSem[iTag], ulWait);
        if (ulRet != ERROR_NONE) {                                      /* 操作失败                     */
            AHCI_CMD_LOG(AHCI_LOG_ERR,
                         "sync sem timeout ata cmd %02x ctrl %d drive %d.",
                         hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand, hCtrl->AHCICTRL_uiIndex, uiDrive);
            /*
             *  发送错误消息以便监测线程进行处理
             */
            hDrive->AHCIDRIVE_uiTimeoutErrorCount++;
            hCtrlMsg->AHCIMSG_uiMsgId = AHCI_MSG_ERROR;
            hCtrlMsg->AHCIMSG_uiDrive = uiDrive;
            hCtrlMsg->AHCIMSG_hCtrl   = hCtrl;
            API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)hCtrlMsg, AHCI_MSG_SIZE);

            __ahciCmdReleaseResource(hDrive, bQueued);                  /* 释放使用权                   */
            return  (PX_ERROR);
        }

        AHCI_CMD_LOG(AHCI_LOG_PRT, "state check.", 0);
        if (((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
             (hDrive->AHCIDRIVE_ucState != AHCI_DEV_INIT)) ||
            (hDrive->AHCIDRIVE_bPortError)) {                           /* 控制器状态错误               */
            AHCI_CMD_LOG(AHCI_LOG_ERR,
                         "port error cmd %02x ctrl %d drive %d.",
                         hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand, hCtrl->AHCICTRL_uiIndex, uiDrive);

            __ahciCmdReleaseResource(hDrive, bQueued);                  /* 是否使用权                   */
            return  (PX_ERROR);
        }

        if (hCmd->AHCICMD_iDirection == AHCI_DATA_DIR_IN) {             /* 输入模式                     */
            AHCI_CMD_LOG(AHCI_LOG_PRT, "read buffer invalidate.", 0);
                                                                        /* 无效数据区                   */
            API_CacheDmaInvalidate(hCmd->AHCICMD_pucDataBuf, (size_t)hCmd->AHCICMD_ulDataLen);
        }
    }

    __ahciCmdReleaseResource(hDrive, bQueued);                          /* 是否控制权                   */

    return  (ERROR_NONE);                                               /* 正确返回                     */
}
/*********************************************************************************************************
** 函数名称: __ahciDiskAtaParamGet
** 功能描述: 获取 ATA 参数
** 输　入  : hCtrl      控制器句柄
**           iDrive     驱动器号
**           pvBuff     参数缓冲区
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDiskAtaParamGet (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive, PVOID  pvBuff)
{
    INT                 iRet   = PX_ERROR;                              /* 操作结果                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* 控制器句柄                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* 命令控制块                   */
    AHCI_CMD_HANDLE     hCmd   = LW_NULL;                               /* 命令句柄                     */

    AHCI_LOG(AHCI_LOG_PRT, "ata parameter get.", 0);

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* 获得驱动器句柄               */
    hCmd = &tCtrlCmd;                                                   /* 获取命令句柄                 */
    if ((!hCtrl->AHCICTRL_bDrvInstalled) ||
        (!hCtrl->AHCICTRL_bInstalled)) {
        return  (PX_ERROR);
    }
    
    /*
     *  构造命令
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen = 512;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_READP;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_NON_SEC_DATA;

    lib_bzero(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);
    iRet = __ahciDiskCommandSend(hCtrl, iDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    lib_memcpy(pvBuff, hDrive->AHCIDRIVE_pucAlignDmaBuf, 512);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciDiskAtapiParamGet
** 功能描述: 获取 ATAPI 参数
** 输　入  : hCtrl      控制器句柄
**           iDrive     驱动器号
**           pvBuf      参数缓冲区
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDiskAtapiParamGet (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive, PVOID  pvBuf)
{
    INT                 iRet   = PX_ERROR;                              /* 操作结果                     */
    AHCI_DRIVE_HANDLE   hDrive = LW_NULL;                               /* 控制器句柄                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* 命令控制块                   */
    AHCI_CMD_HANDLE     hCmd   = LW_NULL;                               /* 命令句柄                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* 获得驱动器句柄               */
    hCmd = &tCtrlCmd;                                                   /* 获取命令句柄                 */
    
    /*
     *  构造命令
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCICMD_ulDataLen = 512;
    hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_PI_CMD_IDENTD;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = 0;
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_NON_SEC_DATA;

    iRet = __ahciDiskCommandSend(hCtrl, iDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    lib_memcpy(pvBuf, hDrive->AHCIDRIVE_pucAlignDmaBuf, 512);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciLbaRangeEntriesSet
** 功能描述: 设置扇区条目
** 输　入  : hDev           设备句柄
**           ulStartLba     起始扇区
**           ulSectors      扇区数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if AHCI_TRIM_EN > 0                                                    /* AHCI_TRIM_EN                 */

static ULONG  __ahciLbaRangeEntriesSet (PVOID  pvBuffer, ULONG  ulBuffSize, ULONG ulSector, ULONG ulCount)
{
    REGISTER INT    i = 0;
    UINT64         *pullBuffer;
    UINT64          ullEntry = 0;
    ULONG           ulBytes = 0;

    /*
     *  6-byte LBA + 2-byte range per entry
     */
    pullBuffer = (UINT64 *)pvBuffer;
    while (i < ulBuffSize / AHCI_TRIM_TCB_SIZE) {
        ullEntry = ulSector |
                   ((UINT64)(ulCount > AHCI_TRIM_BLOCK_MAX ? AHCI_TRIM_BLOCK_MAX : ulCount) << 48);

        pullBuffer[i++] = AHCI_SWAP64(ullEntry);
        if (ulCount <= AHCI_TRIM_BLOCK_MAX) {
            break;
        }
        ulCount -= AHCI_TRIM_BLOCK_MAX;
        ulSector += AHCI_TRIM_BLOCK_MAX;
    }

    ulBytes = ALIGN(i * AHCI_TRIM_TCB_SIZE, (AHCI_TRIM_TCB_SIZE * AHCI_TRIM_TCB_MAX));
    lib_bzero(pullBuffer + i, ulBytes - (i * AHCI_TRIM_TCB_SIZE));

    return  (ulBytes);
}
/*********************************************************************************************************
** 函数名称: __ahciDiskTrimSet
** 功能描述: TRIM 操作
** 输　入  : hDev               设备句柄
**           ulStartSector      起始扇区
**           ulEndSector        结束扇区
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDiskTrimSet (AHCI_DEV_HANDLE  hDev, ULONG  ulStartSector, ULONG  ulEndSector)
{
    INT                 iRet;                                           /* 操作结果                     */
    REGISTER INT        i;                                              /* 循环因子                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* 驱动器句柄                   */
    ULONG               ulDataLen;                                      /* 数据长度                     */
    AHCI_CMD_CB         tCtrlCmd;                                       /* 命令控制块                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* 命令句柄                     */
    UINT32              uiCmdCount;                                     /* 命令条目数量                 */
    UINT32              uiSector;                                       /* 单次扇区数                   */
    ULONG               ulSectors;                                      /* 总扇区数                     */

    hDrive = hDev->AHCIDEV_hDrive;                                      /* 获取驱动器句柄               */
    hCmd = &tCtrlCmd;                                                   /* 获取命令句柄                 */

    if (hDrive->AHCIDRIVE_bTrim != LW_TRUE) {                           /* TRIM 不支持                  */
        AHCI_LOG(AHCI_LOG_PRT, "trim not support.", 0);
        return  (ERROR_NONE);
    }

    AHCI_LOG(AHCI_LOG_PRT,
             "trim ctrl %d drive %d start %lu end %lu offset %lu count %lu.",
             hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
             ulStartSector, ulEndSector, hDev->AHCIDEV_ulBlkOffset, hDev->AHCIDEV_ulBlkCount);
    /*
     *  监测扇区参数是否正确
     */
    if ((ulStartSector > ulEndSector) ||
        (ulStartSector < hDev->AHCIDEV_ulBlkOffset) ||
        (ulEndSector > (hDev->AHCIDEV_ulBlkOffset + hDev->AHCIDEV_ulBlkCount - 1))) {
        AHCI_LOG(AHCI_LOG_ERR,
                 "sector error start sector %lu end sector %lu [%lu ~ %lu].\n",
                 ulStartSector, ulEndSector,
                 hDev->AHCIDEV_ulBlkOffset, (hDev->AHCIDEV_ulBlkOffset + hDev->AHCIDEV_ulBlkCount - 1));
        return  (PX_ERROR);
    }
    /*
     *  计算扇区参数
     */
    ulSectors = ulEndSector - ulStartSector + 1;
    uiCmdCount = (ulSectors + AHCI_TRIM_CMD_BLOCK_MAX - 1) / AHCI_TRIM_CMD_BLOCK_MAX;
    AHCI_LOG(AHCI_LOG_PRT, "sectors %lu cmd count %lu.\n", ulSectors, uiCmdCount);
    for (i = 0; i < uiCmdCount; i++) {
        uiSector = (ulSectors >= AHCI_TRIM_CMD_BLOCK_MAX) ? AHCI_TRIM_CMD_BLOCK_MAX : ulSectors;
        ulDataLen = __ahciLbaRangeEntriesSet(hDrive->AHCIDRIVE_pucAlignDmaBuf,
                                             hDrive->AHCIDRIVE_ulByteSector,
                                             ulStartSector, uiSector);
#if (AHCI_LOG_PRT & AHCI_LOG_LEVEL)                                     /* AHCI_LOG_LEVEL               */
        {
            REGISTER INT        j;
            
            API_CacheDmaFlush(hDrive->AHCIDRIVE_pucAlignDmaBuf, hDrive->AHCIDRIVE_ulByteSector);

            AHCI_LOG(AHCI_LOG_PRT,
                     "start sector %lu one sector %lu sectors %lu data len %lu.",
                     ulStartSector, uiSector, ulSectors, ulDataLen);
            AHCI_LOG(AHCI_LOG_PRT,
                     "start sector %08x one sector %08x sectors %08x data len %08x.",
                     ulStartSector, uiSector, ulSectors, ulDataLen);
            
            for (j = 0; j < hDrive->AHCIDRIVE_ulByteSector / 8; j++) {
                AHCI_LOG(AHCI_LOG_PRT,
                         "%02d  %02x%02x %02x%02x%02x%02x%02x%02x",
                         j,
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 7],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 6],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 5],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 4],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 3],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 2],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 1],
                         hDrive->AHCIDRIVE_pucAlignDmaBuf[j * 8 + 0]);
            }
        }
#endif                                                                  /* AHCI_LOG_LEVEL               */
        /*
         *  构造命令
         */
        lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
        hCmd->AHCICMD_pucDataBuf = hDrive->AHCIDRIVE_pucAlignDmaBuf;
        hCmd->AHCICMD_ulDataLen = ulDataLen;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_DSM;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = 1;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = 0x01;
        hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = 0;
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_OUT;
        hCmd->AHCICMD_iFlags = 0;

        AHCI_LOG(AHCI_LOG_PRT, "send trim cmd\n", 0);
        iRet = __ahciDiskCommandSend(hDev->AHCIDEV_hCtrl, hDev->AHCIDEV_uiDrive, hCmd);
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "trim failed\n", 0);
            return  (PX_ERROR);
        }

        ulSectors -= uiSector;
        ulStartSector += uiSector;
    }

    return  (ERROR_NONE);
}

#endif                                                                  /* AHCI_TRIM_EN                 */
/*********************************************************************************************************
** 函数名称: API_AhciDiskTrimSet
** 功能描述: 从指定扇区读写数据
** 输　入  : hCtrl          控制器句柄
**           uiDrive        驱动器号
**           ullStartLba    起始扇区
**           ullSectors     扇区数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciDiskTrimSet (AHCI_CTRL_HANDLE  hCtrl,
                          UINT              uiDrive,
                          ULONG             ulStartSector,
                          ULONG             ulEndSector)
{
#if (AHCI_TRIM_EN > 0)                                                  /* AHCI_TRIM_EN                 */
    INT                 iRet;
    AHCI_DRIVE_HANDLE   hDrive;

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];

    iRet = __ahciDiskTrimSet(hDrive->AHCIDRIVE_hDev, ulStartSector, ulEndSector);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
#endif                                                                  /* AHCI_TRIM_EN                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciReadWrite
** 功能描述: 从指定扇区读写数据
** 输　入  : hCtrl          控制器句柄
**           uiDrive        驱动器号
**           pvBuf          数据缓冲区
**           ulLba          起始扇区
**           ulSectors      扇区数
**           uiDirection    数据传输方向
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static INT  __ahciReadWrite (AHCI_CTRL_HANDLE  hCtrl,
                             UINT              uiDrive,
                             PVOID             pvBuf,
                             ULONG             ulLba,
                             ULONG             ulSectors,
                             UINT              uiDirection)
{
    INT                 iRet;                                           /* 操作结果                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* 驱动器句柄                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* 命令控制块                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* 命令句柄                     */

    hDrive = &hCtrl->AHCICTRL_hDrive[uiDrive];                          /* 获取驱动器句柄               */
    hCmd = &tCtrlCmd;                                                   /* 获取命令句柄                 */
    /*
     *  构造命令
     */
    hCmd->AHCICMD_ulDataLen = ulSectors * hDrive->AHCIDRIVE_ulByteSector;
    hCmd->AHCICMD_pucDataBuf = (UINT8 *)pvBuf;
    hCmd->AHCICMD_iFlags = 0;
    /*
     *  获取操作模式
     */
    if (uiDirection == O_WRONLY) {
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_OUT;
        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                      (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_MULTI_EXT : AHCI_CMD_WRITE_MULTI);
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                                  (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_EXT : AHCI_CMD_WRITE);
            }
        
        } else {
            if (hDrive->AHCIDRIVE_bNcq) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =  AHCI_CMD_WRITE_FPDMA_QUEUED;
                hCmd->AHCICMD_iFlags |= AHCI_CMD_FLAG_NCQ;
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                          (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_WRITE_DMA_EXT : AHCI_CMD_WRITE_DMA);
            }
        }
    
    } else {
        hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_IN;
        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                        (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_MULTI_EXT : AHCI_CMD_READ_MULTI);
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                                    (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_EXT : AHCI_CMD_READ);
            }
        
        } else {
            if (hDrive->AHCIDRIVE_bNcq) {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = AHCI_CMD_READ_FPDMA_QUEUED;
                hCmd->AHCICMD_iFlags |= AHCI_CMD_FLAG_NCQ;
            } else {
                hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand =
                            (UINT8)(hDrive->AHCIDRIVE_bLba48 ? AHCI_CMD_READ_DMA_EXT : AHCI_CMD_READ_DMA);
            }
        }
    }
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = (UINT16)ulSectors;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature= 0;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = (UINT64)ulLba;

    iRet = __ahciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciBlkReadWrite
** 功能描述: 块设备读写操作
** 输　入  : hDev           设备句柄
**           pvBuffer       缓冲区地址
**           ulBlkStart     起始
**           ulBlkCount     数量
**           uiDirection    读写模式
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciBlkReadWrite (AHCI_DEV_HANDLE  hDev,
                                PVOID            pvBuffer,
                                ULONG            ulBlkStart,
                                ULONG            ulBlkCount,
                                UINT             uiDirection)
{
    REGISTER UINT32     i;                                              /* 循环因子                     */
    INT                 iRet = PX_ERROR;                                /* 操作结果                     */
    AHCI_CTRL_HANDLE    hCtrl;                                          /* 控制器句柄                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* 驱动器句柄                   */
    PLW_BLK_DEV         hBlkdev;                                        /* 块设备句柄                   */
    INT                 iRetry  = 0;                                    /* 重试参数                     */
    ULONG               ulLba   = 0;                                    /* LBA 参数                     */
    ULONG               ulSector = 0;                                   /* 扇区参数                     */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* 获取控制器句柄               */
    hDrive = hDev->AHCIDEV_hDrive;                                      /* 获取驱动器句柄               */
    hBlkdev = &hDev->AHCIDEV_tBlkDev;                                   /* 获取块设备句柄               */
    if ((hCtrl->AHCICTRL_bInstalled == LW_FALSE) ||
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK)) {                   /* 状态错误                     */
        return  (PX_ERROR);
    }

    AHCI_CMD_LOG(AHCI_LOG_PRT,
                 "ctrl %d drive %d start %d blks %d buff %p dir %d\n",
                 hDev->AHCIDEV_uiCtrl, hDev->AHCIDEV_uiDrive,
                 ulBlkStart, ulBlkCount, pvBuffer, uiDirection);

    iRet = API_AhciPmActive(hCtrl, hDev->AHCIDEV_uiDrive);              /* 使能电源                     */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    ulSector = (ULONG)hBlkdev->BLKD_ulNSector;
    if ((ulBlkStart + ulBlkCount) > (ULONG)ulSector) {                  /* 扇区参数错误                 */
        AHCI_LOG(AHCI_LOG_ERR, "start blk %lu blks %d [0 ~ %lu]\n", ulBlkStart, ulBlkCount, ulSector);
        return  (PX_ERROR);
    }

    ulBlkStart += (ULONG)hDev->AHCIDEV_ulBlkOffset;
    for (i = 0; i < (UINT32)ulBlkCount; i += ulSector) {                /* 循环处理所有扇区             */
        iRetry = 0;
        ulLba = (ULONG)ulBlkStart;

        if (hDrive->AHCIDRIVE_usRwMode < AHCI_DMA_MULTI_0) {
            if (hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) {
                ulSector = __MIN((UINT32)ulBlkCount - i, (UINT32)hDrive->AHCIDRIVE_usMultiSector);
            } else {
                ulSector = 1;
            }
        } else if ((hDrive->AHCIDRIVE_bLba48) ||
                   (hDrive->AHCIDRIVE_bLba)) {
            ulSector = __MIN((UINT32)ulBlkCount - i, (UINT32)hDrive->AHCIDRIVE_ulSectorMax);
        }

        if (uiDirection == O_WRONLY) {
            lib_memcpy(&hDev->AHCIDEV_pucAlignDmaBuf[0], pvBuffer,
                       (hDrive->AHCIDRIVE_ulByteSector * ulSector));
            API_CacheDmaFlush((PVOID)&hDev->AHCIDEV_pucAlignDmaBuf[0],
                              (size_t)(hDrive->AHCIDRIVE_ulByteSector * ulSector));
        }
        iRet = __ahciReadWrite(hCtrl, hDev->AHCIDEV_uiDrive,
                               &hDev->AHCIDEV_pucAlignDmaBuf[0], ulLba, ulSector, uiDirection);
        while (iRet != ERROR_NONE) {                                    /* 错误重试                     */
            if (iRetry > AHCI_RETRY_NUM) {
                goto   __error_handle;
            }
            API_TimeSleep(1);

            iRet = __ahciReadWrite(hCtrl, hDev->AHCIDEV_uiDrive,
                                   &hDev->AHCIDEV_pucAlignDmaBuf[0], ulLba, ulSector, uiDirection);
            iRetry += 1;
        }
        
        if (uiDirection == O_RDONLY) {
            API_CacheDmaInvalidate((PVOID)&hDev->AHCIDEV_pucAlignDmaBuf[0],
                                   (size_t)(hDrive->AHCIDRIVE_ulByteSector * ulSector));
            lib_memcpy(pvBuffer, &hDev->AHCIDEV_pucAlignDmaBuf[0],
                       (size_t)(hDrive->AHCIDRIVE_ulByteSector * ulSector));
        }

        ulBlkStart += (ULONG)ulSector;                                  /* 更新起始扇区                 */
        pvBuffer    = (UINT8 *)pvBuffer
                    + (hBlkdev->BLKD_ulBytesPerSector
                    *  ulSector);                                       /* 更新缓冲区                   */
    }

    return  (ERROR_NONE);

__error_handle:
    _ErrorHandle(EIO);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __ahciBlkReset
** 功能描述: 块设备复位
** 输　入  : hDev  设备句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciBlkReset (AHCI_DEV_HANDLE  hDev)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* 控制器句柄                   */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* 获取控制器句柄               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* 控制器未安装                 */
         return  (PX_ERROR);                                            /* 错误返回                     */
    }

    return  (ERROR_NONE);                                               /* 正确返回                     */
}
/*********************************************************************************************************
** 函数名称: __ahciBlkStatusChk
** 功能描述: 块设备检测
** 输　入  : hDev  设备句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciBlkStatusChk (AHCI_DEV_HANDLE  hDev)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* 控制器句柄                   */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* 获取控制器句柄               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* 控制器未安装                 */
         return  (PX_ERROR);                                            /* 错误返回                     */
    }

    return  (ERROR_NONE);                                               /* 正确返回                     */
}
/*********************************************************************************************************
** 函数名称: __ahciBlkIoctl
** 功能描述: 块设备 ioctl
** 输　入  : hDev   设备句柄
**           iCmd   控制命令
**           lArg   控制参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciBlkIoctl (AHCI_DEV_HANDLE  hDev, INT  iCmd, LONG  lArg)
{
    AHCI_CTRL_HANDLE    hCtrl;                                          /* 控制器句柄                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* 驱动器句柄                   */

    hCtrl = hDev->AHCIDEV_hCtrl;                                        /* 获取控制器句柄               */
    hDrive = hDev->AHCIDEV_hDrive;                                      /* 获取驱动器句柄               */
    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* 控制器未安装                 */
         return  (PX_ERROR);                                            /* 错误返回                     */
    }

    switch (iCmd) {

    /*
     *  必须要支持的命令
     */
    case FIOSYNC:
    case FIODATASYNC:
    case FIOSYNCMETA:
    case FIOFLUSH:                                                      /*  将缓存写入磁盘              */
    case FIOUNMOUNT:                                                    /*  卸载卷                      */
    case FIODISKINIT:                                                   /*  初始化磁盘                  */
    case FIODISKCHANGE:                                                 /*  磁盘媒质发生变化            */
        break;

    case FIOTRIM:
#if AHCI_TRIM_EN > 0                                                    /* AHCI_TRIM_EN                 */
    {
        INT                 iRet;                                       /* 操作结果                     */
        PLW_BLK_RANGE       hBlkRange;                                  /* 块设备扇区范围参数           */

        hBlkRange = (PLW_BLK_RANGE)lArg;
        iRet = __ahciDiskTrimSet(hDev, hBlkRange->BLKR_ulStartSector, hBlkRange->BLKR_ulEndSector);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }
#endif                                                                  /* AHCI_TRIM_EN                 */
        break;
    /*
     *  低级格式化
     */
    case FIODISKFORMAT:                                                 /*  格式化卷                    */
        return  (PX_ERROR);                                             /*  不支持低级格式化            */

    /*
     *  FatFs 扩展命令
     */
    case LW_BLKD_CTRL_POWER:
    case LW_BLKD_CTRL_LOCK:
    case LW_BLKD_CTRL_EJECT:
        break;

    case LW_BLKD_GET_SECSIZE:
    case LW_BLKD_GET_BLKSIZE:
        *((LONG *)lArg) = hDrive->AHCIDRIVE_ulByteSector;
        break;

    case LW_BLKD_GET_SECNUM:
        *((ULONG *)lArg) = (ULONG)API_AhciDriveSectorCountGet(hCtrl, hDev->AHCIDEV_uiDrive);
        break;

    case FIOWTIMEOUT:
    case FIORTIMEOUT:
        break;

    case AHCI_IOCTL_APM_ENABLE:
        return  (API_AhciApmEnable(hCtrl,  hDev->AHCIDEV_uiDrive, (UINT8)lArg));
        break;

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciBlkWr
** 功能描述: 块设备写操作
** 输　入  : hDev           设备句柄
**           pvBuffer       缓冲区地址
**           ulBlkStart     起始
**           ulBlkCount     数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciBlkWr (AHCI_DEV_HANDLE  hDev, PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount)
{
    return  (__ahciBlkReadWrite(hDev, pvBuffer, ulBlkStart, ulBlkCount, O_WRONLY));
}
/*********************************************************************************************************
** 函数名称: __ahciBlkRd
** 功能描述: 块设备读操作
** 输　入  : hDev           设备句柄
**           pvBuffer       缓冲区地址
**           ulBlkStart     起始
**           ulBlkCount     数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciBlkRd (AHCI_DEV_HANDLE  hDev, PVOID  pvBuffer, ULONG  ulBlkStart, ULONG  ulBlkCount)
{
    return  (__ahciBlkReadWrite(hDev, pvBuffer, ulBlkStart, ulBlkCount, O_RDONLY));
}
/*********************************************************************************************************
** 函数名称: __ahciBlkDevRemove
** 功能描述: 移除块设备
** 输　入  : hCtrl      控制器句柄
**           iDrive     驱动器编号
** 输　出  : 块设备控制块句柄
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciBlkDevRemove (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive)
{
    AHCI_LOG(AHCI_LOG_ERR, "blk device remove failed.", 0);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __ahciBlkDevCreate
** 功能描述: 创建块设备
** 输　入  : hCtrl          控制器句柄
**           iDrive         驱动器编号
**           ulBlkOffset    偏移
**           ulBlkCount     数量
** 输　出  : 块设备控制块句柄
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_BLK_DEV  __ahciBlkDevCreate (AHCI_CTRL_HANDLE  hCtrl,
                                        INT               iDrive,
                                        ULONG             ulBlkOffset,
                                        ULONG             ulBlkCount)
{
    INT                 iRet          = PX_ERROR;                       /* 操作结果                     */
    AHCI_DRIVE_HANDLE   hDrive        = LW_NULL;                        /* 驱动器句柄                   */
    AHCI_DRV_HANDLE     hDrv          = LW_NULL;                        /* 驱动句柄                     */
    PLW_BLK_DEV         hBlkDev       = LW_NULL;                        /* 块设备句柄                   */
    AHCI_DEV_HANDLE     hDev          = LW_NULL;                        /* 设备句柄                     */
    UINT64              ullBlkMax     = 0;                              /* 最大扇区数                   */
    ULONG               ulBlkMax      = 0;                              /* 最大扇区数                   */
    ULONG               ulCacheSize   = AHCI_CACHE_SIZE;                /* 缓冲大小                     */
    ULONG               ulBurstSizeRd = AHCI_CACHE_BURST_RD;            /* 猝发读大小                   */
    ULONG               ulBurstSizeWr = AHCI_CACHE_BURST_WR;            /* 猝发写大小                   */

    if (!hCtrl) {                                                       /* 控制器句柄无效               */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl handle.", 0);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE) {                    /* 控制器驱动未安装             */
        AHCI_LOG(AHCI_LOG_ERR, "ahci driver invalid.", 0);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (hCtrl->AHCICTRL_bInstalled == LW_FALSE) {                       /* 控制器未安装                 */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl is not installed.", 0);
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if ((iDrive < 0) ||
        (iDrive >= hCtrl->AHCICTRL_uiImpPortNum)) {                     /* 驱动器索引超限               */
        AHCI_LOG(AHCI_LOG_ERR,
                 "drive %d is out of range (0-%d).", iDrive, (hCtrl->AHCICTRL_uiImpPortNum - 1));
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* 获取驱动器句柄               */
    hDev   = hDrive->AHCIDRIVE_hDev;                                    /* 获得设备句柄                 */
    if (!hDev) {                                                        /* 设备句柄无效                 */
        hDev = (AHCI_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_DEV_CB));    /* 分配设备控制块               */
        if (!hDev) {                                                    /* 分配控制块失败               */
            AHCI_LOG(AHCI_LOG_ERR, "alloc ahci dev tcb failed.", 0);
            return  (LW_NULL);
        }
    }
    hDrive->AHCIDRIVE_hDev = hDev;                                      /* 更新驱动器的设备句柄         */
    hDrv = hCtrl->AHCICTRL_hDrv;                                        /* 获得驱动句柄                 */
    /*
     *  获得扇区缓冲区大小并分配可 DMA 的物理内存
     */
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_SECTOR_BUFF_SIZE_GET,
                                      (LONG)((ULONG *)&hDev->AHCIDEV_ulAlignDmaBufLen));
    if (iRet != ERROR_NONE) {
        hDev->AHCIDEV_ulAlignDmaBufLen = AHCI_SECTOR_BUFF_LEN;
    }
    hDev->AHCIDEV_pucAlignDmaBuf = (UINT8 *)API_CacheDmaMalloc((size_t)(hDev->AHCIDEV_ulAlignDmaBufLen +
                                                                        hDrive->AHCIDRIVE_ulByteSector));
    if (!hDev->AHCIDEV_pucAlignDmaBuf) {
        AHCI_LOG(AHCI_LOG_ERR, "dma alloc sector buff failed.", 0);
        return  (LW_NULL);
    }
    hDev->AHCIDEV_hCtrl       = hCtrl;                                  /* 更新控制器句柄               */
    hDev->AHCIDEV_hDrive      = hDrive;                                 /* 更新驱动器句柄               */
    hDev->AHCIDEV_uiCtrl      = hCtrl->AHCICTRL_uiIndex;                /* 更新控制器全局索引           */
    hDev->AHCIDEV_uiDrive     = iDrive;                                 /* 更新驱动器索引               */
    hDev->AHCIDEV_ulBlkOffset = ulBlkOffset;                            /* 更新扇区偏移                 */

    hBlkDev = &hDev->AHCIDEV_tBlkDev;                                   /* 获得块设备句柄               */
    if ((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH)) {               /* 驱动器状态错误               */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;                      /* 复位驱动器状态               */
        AHCI_LOG(AHCI_LOG_ERR, "drive state error.", 0);
        return  (LW_NULL);
    }

    hDrive->AHCIDRIVE_ucState = AHCI_DEV_MED_CH;                        /* 更新设备状态                 */
    switch (hDrive->AHCIDRIVE_ucType) {                                 /* 检查设备类型                 */

    case AHCI_TYPE_ATA:                                                 /* ATA 设备                     */
        if (hDrive->AHCIDRIVE_bLba48 == LW_TRUE) {                      /* LBA 48 模式                  */
            ullBlkMax = ((hCtrl->AHCICTRL_ullLba48TotalSecs[iDrive]) - (UINT64)ulBlkOffset);
            ulBlkMax = (ULONG)ullBlkMax;
        } else if ((hDrive->AHCIDRIVE_bLba == LW_TRUE) &&
                   (hCtrl->AHCICTRL_uiLbaTotalSecs[iDrive] != 0) &&
                   (hCtrl->AHCICTRL_uiLbaTotalSecs[iDrive] >
                   (UINT32)(hDrive->AHCIDRIVE_uiCylinder *
                            hDrive->AHCIDRIVE_uiHead *
                            hDrive->AHCIDRIVE_uiSector))) {             /* LBA 模式                     */
            ulBlkMax = (ULONG)(hCtrl->AHCICTRL_uiLbaTotalSecs[iDrive] - (UINT32)ulBlkOffset);
        } else {                                                        /* CHS 模式                     */
            ulBlkMax = (ULONG)((hDrive->AHCIDRIVE_uiCylinder *
                                hDrive->AHCIDRIVE_uiHead *
                                hDrive->AHCIDRIVE_uiSector) - ulBlkOffset);
        }
        if ((ulBlkCount == 0) ||
            (ulBlkCount > ulBlkMax)) {                                  /* 全部扇区或超限               */
            hDev->AHCIDEV_ulBlkCount = (ULONG)ulBlkMax;
        }
        /*
         *  配置块设备参数
         */
        hBlkDev->BLKD_pcName            = &hDrive->AHCIDRIVE_cDevName[0];
        hBlkDev->BLKD_ulNSector         = hDev->AHCIDEV_ulBlkCount;
        hBlkDev->BLKD_ulBytesPerSector  = hDrive->AHCIDRIVE_ulByteSector;
        hBlkDev->BLKD_ulBytesPerBlock   = hDrive->AHCIDRIVE_ulByteSector;
        hBlkDev->BLKD_bRemovable        = LW_TRUE;
        hBlkDev->BLKD_iRetry            = 1;
        hBlkDev->BLKD_iFlag             = O_RDWR;
        hBlkDev->BLKD_bDiskChange       = LW_TRUE;
        hBlkDev->BLKD_pfuncBlkRd        = __ahciBlkRd;
        hBlkDev->BLKD_pfuncBlkWrt       = __ahciBlkWr;
        hBlkDev->BLKD_pfuncBlkIoctl     = __ahciBlkIoctl;
        hBlkDev->BLKD_pfuncBlkReset     = __ahciBlkReset;
        hBlkDev->BLKD_pfuncBlkStatusChk = __ahciBlkStatusChk;

        hBlkDev->BLKD_iLogic            = 0;                            /*  物理设备                    */
        hBlkDev->BLKD_uiLinkCounter     = 0;
        hBlkDev->BLKD_pvLink            = LW_NULL;

        hBlkDev->BLKD_uiPowerCounter    = 0;
        hBlkDev->BLKD_uiInitCounter     = 0;

        hDrive->AHCIDRIVE_ucState = AHCI_DEV_OK;                        /* 更新驱动器状态               */
        /*
         *  获取驱动器块设备参数信息
         */
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, iDrive, AHCI_OPT_CMD_CACHE_SIZE_GET,
                                          (LONG)((ULONG *)&ulCacheSize));
        if (iRet != ERROR_NONE) {
            ulCacheSize = AHCI_CACHE_SIZE;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, iDrive, AHCI_OPT_CMD_CACHE_BURST_RD_GET,
                                          (LONG)((ULONG *)&ulBurstSizeRd));
        if (iRet != ERROR_NONE) {
            ulBurstSizeRd = AHCI_CACHE_BURST_RD;
        }
        iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, iDrive, AHCI_OPT_CMD_CACHE_BURST_WR_GET,
                                          (LONG)((ULONG *)&ulBurstSizeWr));
        if (iRet != ERROR_NONE) {
            ulBurstSizeWr = AHCI_CACHE_BURST_WR;
        }
                                                                        /* 挂载设备                     */
        hDev->AHCIDEV_pvOemdisk = (PVOID)API_OemDiskMount2(AHCI_MEDIA_NAME,
                                                           hBlkDev,
                                                           LW_NULL,
                                                           ulCacheSize,
                                                           ulBurstSizeRd,
                                                           ulBurstSizeWr);
        if (!hDev->AHCIDEV_pvOemdisk) {                                 /* 挂载失败                     */
            AHCI_LOG(AHCI_LOG_ERR, "oem disk mount failed.", 0);
        }
        return  (hBlkDev);                                              /* 返回块设备句柄               */
        break;

    case AHCI_TYPE_ATAPI:                                               /* ATAPI 设备                   */
        break;

    default:                                                            /* 设备类型错误                 */
        return  (LW_NULL);
        break;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __ahciDiskConfig
** 功能描述: 配置磁盘驱动
** 输　入  : hCtrl          控制器句柄
**           iDrive         驱动器号
**           cpcDevName     设备名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDiskConfig (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive, CPCHAR  cpcDevName)
{
    PLW_BLK_DEV         hBlkDev = LW_NULL;                              /* 块设备句柄                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* 驱动器句柄                   */

    if (!hCtrl) {                                                       /* 控制器句柄无效               */
        AHCI_LOG(AHCI_LOG_ERR, "invalid ctrl handle.", 0);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((!cpcDevName) ||
        (cpcDevName[0] == PX_EOS)) {                                    /* 设备名称无效                 */
        AHCI_LOG(AHCI_LOG_ERR, "invalid device name.", 0);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((hCtrl->AHCICTRL_bInstalled == LW_FALSE) ||
        (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE)) {                  /* 驱动或控制器未安装           */
        AHCI_LOG(AHCI_LOG_ERR, "ctrl or driver is not installed.", 0);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* 获取驱动器句柄               */
    lib_strncpy(&hDrive->AHCIDRIVE_cDevName[0], cpcDevName, AHCI_DEV_NAME_MAX);
    if ((hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) &&
        (hDrive->AHCIDRIVE_ucState != AHCI_DEV_MED_CH)) {               /* 设备状态错误                 */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
        return  (ERROR_NONE);
    }
                                                                        /* 创建块设备                   */
    hBlkDev = __ahciBlkDevCreate(hCtrl, iDrive, hDrive->AHCIDRIVE_ulStartSector, 0);
    if (!hBlkDev) {                                                     /* 创建块设备失败               */
        AHCI_LOG(AHCI_LOG_ERR, "create blk dev error %s", cpcDevName);
        return  (PX_ERROR);
    }
    API_AhciDevAdd(hCtrl, iDrive);                                      /* 添加设备                     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciDiskCtrlInit
** 功能描述: 磁盘控制器初始化
** 输　入  : hCtrl      控制器句柄
**           iDrive     驱动器号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDiskCtrlInit (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive)
{
    REGISTER INT        i;                                              /* 循环因子                     */
    INT                 iRet;                                           /* 操作结果                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* 驱动器句柄                   */
    AHCI_CMD_CB         tCtrlCmd;                                       /* 命令控制块                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* 命令句柄                     */
    UINT32              uiReg;                                          /* 寄存器                       */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d disk ctrl init.", hCtrl->AHCICTRL_uiIndex, iDrive);

    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];                           /* 获取驱动器句柄               */
    hCmd   = &tCtrlCmd;                                                 /* 命令句柄                     */

    __ahciCmdWaitForResource(hDrive, LW_FALSE);                         /* 非队列模式等待               */
    hDrive->AHCIDRIVE_iInitActive = LW_TRUE;                            /* 设置驱动器活动状态           */

    iRet = API_AhciDriveEngineStop(hDrive);                             /* 停止 DMA                     */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port engine stop failed.", 0);
        return  (PX_ERROR);
    }

    iRet = API_AhciDriveRecvFisStop(hDrive);                            /* 停止接收处理                 */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port recv fis stop failed.", 0);
        return  (PX_ERROR);
    }

    API_AhciDrivePowerUp(hDrive);                                       /* 电源使能                     */
    /*
     *  设置指定端口
     */
    AHCI_LOG(AHCI_LOG_PRT, "port det reset, partial and slumber disable.", 0);
    AHCI_PORT_WRITE(hDrive, AHCI_PxSCTL, AHCI_PSCTL_DET_RESET | AHCI_PSCTL_IPM_PARSLUM_DISABLED);
    API_TimeMSleep(200);
    AHCI_PORT_WRITE(hDrive, AHCI_PxSCTL, 0);
    iRet = API_AhciDriveRegWait(hDrive,
                                AHCI_PxSSTS, AHCI_PSSTS_DET_MSK, LW_FALSE, AHCI_PSSTS_DET_PHY,
                                1, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "port sctl reset failed.", 0);
        return  (PX_ERROR);
    }

    AHCI_LOG(AHCI_LOG_PRT, "restart ctrl %d drive %d port %d.",
             hCtrl->AHCICTRL_uiIndex, iDrive, hDrive->AHCIDRIVE_uiPort);
    AHCI_LOG(AHCI_LOG_PRT, "port : active, recv fis start, power on, spin up.", 0);
    AHCI_PORT_WRITE(hDrive,
                    AHCI_PxCMD, AHCI_PCMD_ICC_ACTIVE | AHCI_PCMD_FRE | AHCI_PCMD_POD | AHCI_PCMD_SUD);
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 3, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "wait link reactivate failed.", 0);
        return  (PX_ERROR);
    }

    API_AhciDriveRegWait(hDrive, AHCI_PxTFD, AHCI_STAT_ACCESS, LW_FALSE, 0, 20, 50, &uiReg);

    AHCI_LOG(AHCI_LOG_PRT, "port start.", 0);
    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD) | AHCI_PCMD_ST;
    AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
    API_AhciDriveRegWait(hDrive, AHCI_PxTFD, AHCI_STAT_ACCESS, LW_FALSE, 0, 20, 50, &uiReg);

    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_SRST_ON;
    __ahciDiskCommandSend(hCtrl, iDrive, hCmd);
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCI, 0x01, LW_TRUE, 0x01, 3, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    hCmd->AHCICMD_iFlags = AHCI_CMD_FLAG_SRST_OFF;
    __ahciDiskCommandSend(hCtrl, iDrive, hCmd);
    iRet = API_AhciDriveRegWait(hDrive, AHCI_PxCI, 0x01, LW_TRUE, 0x01, 3, 50, &uiReg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d port %d stat 0x%08x.",
             hCtrl->AHCICTRL_uiIndex, iDrive, hDrive->AHCIDRIVE_uiPort,
             AHCI_PORT_READ(hDrive, AHCI_PxTFD));

    for (i = 0; i < hDrive->AHCIDRIVE_uiQueueDepth; i++) {              /* 队列模式                     */
        hDrive->AHCIDRIVE_hSyncBSem[i] = API_SemaphoreBCreate("ahci_sync",
                                                              LW_FALSE,
                                                              (LW_OPTION_WAIT_FIFO |
                                                               LW_OPTION_OBJECT_GLOBAL),
                                                              LW_NULL);
    }

    hDrive->AHCIDRIVE_uiCmdStarted = 0;
    hDrive->AHCIDRIVE_bQueued = LW_FALSE;
    hDrive->AHCIDRIVE_iInitActive = LW_FALSE;

    __ahciCmdReleaseResource(hDrive, LW_FALSE);                         /* 以非队列模式释放             */

    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciDiskDriveInit
** 功能描述: 磁盘驱动器初始化
** 输　入  : hCtrl      控制器句柄
**           iDrive     驱动器号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDiskDriveInit (AHCI_CTRL_HANDLE  hCtrl, INT  iDrive)
{
    INT                 iRet;                                           /* 循环因子                     */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* 驱动器句柄                   */
    AHCI_PARAM_HANDLE   hParam;                                         /* 参数句柄                     */
    AHCI_DRV_HANDLE     hDrv;                                           /* 驱动句柄                     */
    INT                 iConfigType;                                    /* 配置类型                     */
    UINT32              uiReg;                                          /* 寄存器                       */
    UINT16              usDma;                                          /* DMA 模式                     */
    PVOID               pvBuff;                                         /* 缓冲区                       */
    INT                 iFlag;                                          /* 标记                         */

    AHCI_LOG(AHCI_LOG_PRT, "ctrl %d port %d disk drive init.", hCtrl->AHCICTRL_uiIndex, iDrive);
    /*
     *  获取参数信息
     */
    hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];
    hParam = &hDrive->AHCIDRIVE_tParam;
    iConfigType = hCtrl->AHCICTRL_piConfigType[iDrive];

    if ((!hCtrl->AHCICTRL_bDrvInstalled) ||
        (!hCtrl->AHCICTRL_bInstalled)) {                                /* 驱动或控制器未安装           */
        return  (PX_ERROR);
    }
                                                                        /* 等待控制权                   */
    API_SemaphoreMPend(hDrive->AHCIDRIVE_hLockMSem, LW_OPTION_WAIT_INFINITE);
    /*
     *  获取驱动器类型
     */
    uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSIG);
    if (uiReg == AHCI_PSIG_ATAPI) {
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_ATAPI;
    } else {
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_ATA;
    }
    /*
     *  获取驱动参数
     */
    hDrv = hCtrl->AHCICTRL_hDrv;
    iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, iDrive, AHCI_OPT_CMD_SECTOR_SIZE_GET,
                                      (LONG)((ULONG *)&hDrive->AHCIDRIVE_ulByteSector));
    if (iRet != ERROR_NONE) {
        hDrive->AHCIDRIVE_ulByteSector = AHCI_SECTOR_SIZE;
    }
    hDrive->AHCIDRIVE_uiAlignSize = (size_t)API_CacheLine(DATA_CACHE);
    pvBuff = API_CacheDmaMallocAlign((size_t)hDrive->AHCIDRIVE_ulByteSector,
                                     (size_t)hDrive->AHCIDRIVE_uiAlignSize);
    hDrive->AHCIDRIVE_pucAlignDmaBuf = (UINT8 *)pvBuff;
    if (!hDrive->AHCIDRIVE_pucAlignDmaBuf) {
        AHCI_LOG(AHCI_LOG_ERR, "alloc aligned vmm dma buffer failed.", 0);
    } else {
        AHCI_LOG(AHCI_LOG_PRT,
                 "align dma buf addr %p size %lu align size %lu.",
                 hDrive->AHCIDRIVE_pucAlignDmaBuf,
                 hDrive->AHCIDRIVE_ulByteSector, hDrive->AHCIDRIVE_uiAlignSize);
    }

    if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATA) {                    /* ATA 模式                     */
        hDrive->AHCIDRIVE_pfuncReset = __ahciDiskCtrlInit;              /* 复位操作                     */

        iRet = __ahciDiskAtaParamGet(hCtrl, iDrive, (PVOID)hParam);     /* 获取驱动器参数               */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR,
                     "ctrl %d drive %d read ata parameters failed.", hCtrl->AHCICTRL_uiIndex, iDrive);
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_PREAD_F;
            goto  __error_handle;
        }
                                                                        /* 诊断设备                     */
        iRet = API_AhciNoDataCommandSend(hCtrl, iDrive, AHCI_CMD_DIAGNOSE, 0, 0, 0, 0, 0, 0);
        if (iRet != ERROR_NONE) {
            API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);
            AHCI_LOG(AHCI_LOG_ERR, "no data command failed.", 0);
            return  (PX_ERROR);
        }
        /*
         *  获取扇区参数信息
         */
        hDrive->AHCIDRIVE_uiCylinder = hParam->AHCIPARAM_usCylinders - 1;
        hDrive->AHCIDRIVE_uiHead     = hParam->AHCIPARAM_usHeads;
        hDrive->AHCIDRIVE_uiSector   = hParam->AHCIPARAM_usSectors;
        if (hParam->AHCIPARAM_usCapabilities & 0x0200) {
            hCtrl->AHCICTRL_uiLbaTotalSecs[iDrive] =
                                (UINT32)((((UINT32)((hParam->AHCIPARAM_usSectors0) & 0x0000ffff)) <<  0) |
                                         (((UINT32)((hParam->AHCIPARAM_usSectors1) & 0x0000ffff)) << 16));
        }
    
    } else if (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATAPI) {           /* ATAPI 类型                   */
        hDrive->AHCIDRIVE_pfuncReset = __ahciDiskCtrlInit;              /* 控制器初始化                 */

        iRet = __ahciDiskAtapiParamGet(hCtrl, iDrive, (PVOID)hParam);   /* 获取 ATAPI 参数              */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "read atapi parameters failed.", 0);
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_PREAD_F;
            goto  __error_handle;
        }
    }
    /*
     *  更新参数信息
     */
    hDrive->AHCIDRIVE_usMultiSector = hParam->AHCIPARAM_usMultiSecs & 0x00ff;
    hDrive->AHCIDRIVE_bMulti        = (hDrive->AHCIDRIVE_usMultiSector != 0) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bIordy        = (hParam->AHCIPARAM_usCapabilities & 0x0800) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bLba          = (hParam->AHCIPARAM_usCapabilities & 0x0200) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_bDma          = (hParam->AHCIPARAM_usCapabilities & 0x0100) ? LW_TRUE : LW_FALSE;

    if ((hCtrl->AHCICTRL_bNcq == LW_TRUE) &&
        (iConfigType & AHCI_NCQ_MODE)) {
        hDrive->AHCIDRIVE_bNcq = (hParam->AHCIPARAM_usSataCapabilities & 0x0100) ? LW_TRUE : LW_FALSE;
    } else {
        hDrive->AHCIDRIVE_bNcq = LW_FALSE;
    }
    
    if (hDrive->AHCIDRIVE_bNcq) {
        hDrive->AHCIDRIVE_uiQueueDepth = __MIN((hParam->AHCIPARAM_usQueueDepth + 1),
                                               hCtrl->AHCICTRL_uiCmdSlotNum);
    } else {
        hDrive->AHCIDRIVE_uiQueueDepth = 1;
    }

    if (hParam->AHCIPARAM_usFeaturesEnabled1 & 0x0400) {
        hDrive->AHCIDRIVE_bLba48 = LW_TRUE;
        hCtrl->AHCICTRL_ullLba48TotalSecs[iDrive] =
                            (UINT64)((((UINT64)((hParam->AHCIPARAM_usLba48Size[0]) & 0x0000ffff)) <<  0) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[1]) & 0x0000ffff)) << 16) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[2]) & 0x0000ffff)) << 24) |
                                     (((UINT64)((hParam->AHCIPARAM_usLba48Size[3]) & 0x0000ffff)) << 32));
        hDrive->AHCIDRIVE_ulSectorMax = AHCI_MAX_RW_48LBA_SECTORS;
    
    } else {
        hDrive->AHCIDRIVE_bLba48 = LW_FALSE;
        hCtrl->AHCICTRL_ullLba48TotalSecs[iDrive] = (UINT64)0;
        hDrive->AHCIDRIVE_ulSectorMax = AHCI_MAX_RW_SECTORS;
    }
    /*
     *  获取工作模式
     */
    hDrive->AHCIDRIVE_usPioMode = (UINT16)((hParam->AHCIPARAM_usPioMode >> 8) & 0x03);
    if (hDrive->AHCIDRIVE_usPioMode > 2) {
        hDrive->AHCIDRIVE_usPioMode = 0;
    }
    if ((hDrive->AHCIDRIVE_bIordy) &&
        (hParam->AHCIPARAM_usValid & 0x02)) {
        if (hParam->AHCIPARAM_usAdvancedPio & 0x01) {
            hDrive->AHCIDRIVE_usPioMode = 3;
        }
        if (hParam->AHCIPARAM_usAdvancedPio & 0x02) {
            hDrive->AHCIDRIVE_usPioMode = 4;
        }
    }

    if ((hDrive->AHCIDRIVE_bDma == LW_TRUE) &&
        (hParam->AHCIPARAM_usValid & 0x02)) {
        hDrive->AHCIDRIVE_usSingleDmaMode = (UINT16)((hParam->AHCIPARAM_usDmaMode >> 8) & 0x03);

        if (hDrive->AHCIDRIVE_usSingleDmaMode >= 2) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 0;
        }
        hDrive->AHCIDRIVE_usMultiDmaMode = 0;

        if (hParam->AHCIPARAM_usSingleDma & 0x04) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 2;
        } else if (hParam->AHCIPARAM_usSingleDma & 0x02) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 1;
        } else if (hParam->AHCIPARAM_usSingleDma & 0x01) {
            hDrive->AHCIDRIVE_usSingleDmaMode = 0;
        }

        if (hParam->AHCIPARAM_usMultiDma & 0x04) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 2;
        } else if (hParam->AHCIPARAM_usMultiDma & 0x02) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 1;
        } else if (hParam->AHCIPARAM_usMultiDma & 0x01) {
            hDrive->AHCIDRIVE_usMultiDmaMode = 0;
        }

        if (hParam->AHCIPARAM_usUltraDma & 0x4000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 6;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x2000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 5;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x1000) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 4;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0800) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 3;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0400) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 2;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0200) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 1;
        } else if (hParam->AHCIPARAM_usUltraDma & 0x0100) {
            hDrive->AHCIDRIVE_usUltraDmaMode = 0;
        }
    }

    hDrive->AHCIDRIVE_usRwPio = (UINT16)(iConfigType & AHCI_PIO_MASK);
    hDrive->AHCIDRIVE_usRwDma = (UINT16)(iConfigType & AHCI_DMA_MASK);
    hDrive->AHCIDRIVE_usRwMode = AHCI_PIO_DEF_W;

    switch (iConfigType & AHCI_MODE_MASK) {

    case AHCI_PIO_0:
    case AHCI_PIO_1:
    case AHCI_PIO_2:
    case AHCI_PIO_3:
    case AHCI_PIO_4:
    case AHCI_PIO_DEF_0:
    case AHCI_PIO_DEF_1:
        hDrive->AHCIDRIVE_usRwMode = (UINT16)(iConfigType & AHCI_MODE_MASK);
        break;

    case AHCI_PIO_AUTO:
        hDrive->AHCIDRIVE_usRwMode = (UINT16)(AHCI_PIO_W_0 + hDrive->AHCIDRIVE_usPioMode);
        break;

    case AHCI_DMA_0:
    case AHCI_DMA_1:
    case AHCI_DMA_2:
    case AHCI_DMA_3:
    case AHCI_DMA_4:
    case AHCI_DMA_5:
    case AHCI_DMA_6:
        if (hDrive->AHCIDRIVE_bDma) {
            usDma = (UINT16)((iConfigType & AHCI_MODE_MASK) - AHCI_DMA_0);
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_SINGLE) {
                if (usDma > hDrive->AHCIDRIVE_usSingleDmaMode) {
                    usDma = hDrive->AHCIDRIVE_usSingleDmaMode;
                }
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_SINGLE_0 + usDma;
            } else if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_MULTI) {
                if (usDma > hDrive->AHCIDRIVE_usMultiDmaMode) {
                    usDma = hDrive->AHCIDRIVE_usMultiDmaMode;
                }
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + usDma;
            } else if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_ULTRA) {
                if (hParam->AHCIPARAM_usUltraDma == 0) {
                    if (usDma > hDrive->AHCIDRIVE_usMultiDmaMode) {
                        usDma = hDrive->AHCIDRIVE_usMultiDmaMode;
                    }
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + usDma;
                } else {
                    if (usDma > hDrive->AHCIDRIVE_usUltraDmaMode) {
                        usDma = hDrive->AHCIDRIVE_usUltraDmaMode;
                    }
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_ULTRA_0 + usDma;
                }
            }
        } else {
            hDrive->AHCIDRIVE_usRwMode = AHCI_PIO_W_0 + hDrive->AHCIDRIVE_usPioMode;
        }
        break;

    case AHCI_DMA_AUTO:
        if (hDrive->AHCIDRIVE_bDma) {
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_SINGLE) {
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_SINGLE_0 + hDrive->AHCIDRIVE_usSingleDmaMode;
            }
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_MULTI) {
                hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + hDrive->AHCIDRIVE_usMultiDmaMode;
            }
            if (hDrive->AHCIDRIVE_usRwDma == AHCI_DMA_ULTRA) {
                if (hParam->AHCIPARAM_usUltraDma != 0) {
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_ULTRA_0 + hDrive->AHCIDRIVE_usUltraDmaMode;
                } else {
                    hDrive->AHCIDRIVE_usRwMode = AHCI_DMA_MULTI_0 + hDrive->AHCIDRIVE_usMultiDmaMode;
                    hDrive->AHCIDRIVE_usRwDma = AHCI_DMA_MULTI;
                }
            }
        }
        break;

    default:
        break;
    }

    hDrive->AHCIDRIVE_bTrim = (hParam->AHCIPARAM_usDataSetManagement & 0x01) ? LW_TRUE : LW_FALSE;
    hDrive->AHCIDRIVE_usTrimBlockNumMax = hParam->AHCIPARAM_usTrimBlockNumMax;
    iFlag = (hParam->AHCIPARAM_usAdditionalSupported >> 14) & 0x01;
    hDrive->AHCIDRIVE_bDrat = iFlag ? LW_TRUE : LW_FALSE;
    iFlag = (hParam->AHCIPARAM_usAdditionalSupported >> 5) & 0x01;
    hDrive->AHCIDRIVE_bRzat = iFlag ? LW_TRUE : LW_FALSE;

    API_AhciDriveInfoShow(hCtrl, iDrive, hParam);                       /* 打印驱动器信息               */
                                                                        /* 设置传输模式                 */
    API_AhciNoDataCommandSend(hCtrl, iDrive,
                              AHCI_CMD_SET_FEATURE, AHCI_SUB_SET_RWMODE,
                              (UINT8)hDrive->AHCIDRIVE_usRwMode, 0, 0, 0, 0);
    /*
     *  设置特性参数
     */
    if (hParam->AHCIPARAM_usFeaturesSupported0 & AHCI_LOOK_SUPPORTED) {
        API_AhciNoDataCommandSend(hCtrl, iDrive, AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_LOOK, 0,0,0,0,0);
    }
    if (hParam->AHCIPARAM_usFeaturesSupported0 & AHCI_WCACHE_SUPPORTED) {
        API_AhciNoDataCommandSend(hCtrl, iDrive, AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_WCACHE, 0,0,0,0,0);
    }
    if (hParam->AHCIPARAM_usFeaturesSupported1 & AHCI_APM_SUPPORT_APM) {
        API_AhciNoDataCommandSend(hCtrl, iDrive, AHCI_CMD_SET_FEATURE, AHCI_SUB_ENABLE_APM, 0xFE,0,0,0,0);
    }

    if ((hDrive->AHCIDRIVE_usRwPio == AHCI_PIO_MULTI) &&
        (hDrive->AHCIDRIVE_ucType == AHCI_TYPE_ATA)) {
        if (hDrive->AHCIDRIVE_bMulti) {
            API_AhciNoDataCommandSend(hCtrl, iDrive,
                                      AHCI_CMD_SET_MULTI, 0, hDrive->AHCIDRIVE_usMultiSector, 0, 0, 0, 0);
        } else {
            hDrive->AHCIDRIVE_usRwPio = AHCI_PIO_SINGLE;
        }
    }

    hDrive->AHCIDRIVE_ucState = AHCI_DEV_OK;
    hDrive->AHCIDRIVE_iPwmState = AHCI_PM_ACTIVE_IDLE;

__error_handle:
    API_SemaphoreMPost(hDrive->AHCIDRIVE_hLockMSem);

    if (hDrive->AHCIDRIVE_ucState != AHCI_DEV_OK) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d drive %d state %d status 0x%x error 0x%x.",
                 hCtrl->AHCICTRL_uiIndex, iDrive,
                 hDrive->AHCIDRIVE_ucState, hDrive->AHCIDRIVE_uiIntStatus, hDrive->AHCIDRIVE_uiIntError);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciDriveNoBusyWait
** 功能描述: 等待驱动器不忙, 机械硬盘从上电到状态正确需要一段时间
** 输　入  : hDrive    驱动器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDriveNoBusyWait (AHCI_DRIVE_HANDLE  hDrive)
{
    REGISTER INT    i;
    UINT32          uiReg;

    for (i = 0; i < hDrive->AHCIDRIVE_uiProbTimeCount; i++) {
        uiReg = AHCI_PORT_READ(hDrive, AHCI_PxTFD);
        if (uiReg & AHCI_STAT_ACCESS) {
            API_TimeMSleep(hDrive->AHCIDRIVE_uiProbTimeUnit);
        } else {
            break;
        }
    }

    if (i >= hDrive->AHCIDRIVE_uiProbTimeCount) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ahciDrvInit
** 功能描述: 初始化 AHCI 驱动
** 输　入  : hCtrl     控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ahciDrvInit (AHCI_CTRL_HANDLE  hCtrl)
{
    REGISTER INT            i;                                          /* 循环因子                     */
    REGISTER INT            j;                                          /* 循环因子                     */
    INT                     iRet;                                       /* 操作结果                     */
    LW_CLASS_THREADATTR     threadattr;                                 /* 线程控制块                   */
    AHCI_DRIVE_HANDLE       hDrive;                                     /* 驱动器句柄                   */
    AHCI_DRV_HANDLE         hDrv;                                       /* 驱动句柄                     */
    size_t                  stSizeTemp;                                 /* 内存大小                     */
    size_t                  stMemSize;                                  /* 内存大小                     */
    UINT8                  *pucCmdList;                                 /* 命令列表                     */
    UINT8                  *pucRecvFis;                                 /* 接收列表                     */
    INT                     iCurrPort;                                  /* 当前端口                     */
    UINT32                  uiPortMap;                                  /* 端口映射信息                 */
    UINT32                  uiReg;                                      /* 寄存器                       */

    AHCI_LOG(AHCI_LOG_PRT, "init ctrl %d name %s uint index %d reg addr 0x%llx.",
             hCtrl->AHCICTRL_uiIndex, hCtrl->AHCICTRL_cCtrlName, hCtrl->AHCICTRL_uiUnitIndex,
             hCtrl->AHCICTRL_pvRegAddr);

    hDrv = hCtrl->AHCICTRL_hDrv;                                        /* 获得驱动句柄                 */

    if (hCtrl->AHCICTRL_bDrvInstalled == LW_FALSE) {                    /* 驱动未安装                   */
        if (hDrv->AHCIDRV_pfuncVendorPlatformInit) {
            iRet = hDrv->AHCIDRV_pfuncVendorPlatformInit(hCtrl);
        } else {
            iRet = ERROR_NONE;
        }

        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d vendor platform init failed.", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bDrvInstalled = LW_TRUE;                        /* 标识驱动安装                 */
    }

    if (hCtrl->AHCICTRL_bMonitorStarted == LW_FALSE) {                  /* 监测线程未创建               */
        hCtrl->AHCICTRL_hMsgQueue = API_MsgQueueCreate("ahci_msg",
                                                       AHCI_MSG_QUEUE_SIZE, AHCI_MSG_SIZE,
                                                       LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);
        if (hCtrl->AHCICTRL_hMsgQueue == LW_OBJECT_HANDLE_INVALID) {    /* 创建监测消息失败             */
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d create ahci msg queue failed.", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }

        /*
         * 创建监测线程
         */
        API_ThreadAttrBuild(&threadattr,
                            AHCI_MONITOR_STK_SIZE, AHCI_MONITOR_PRIORITY,
                            LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL,
                            LW_NULL);
        threadattr.THREADATTR_pvArg = (PVOID)hCtrl;
        hCtrl->AHCICTRL_hMonitorThread = API_ThreadCreate("t_ahcimsg",
                                                          (PTHREAD_START_ROUTINE)__ahciMonitorThread,
                                                          (PLW_CLASS_THREADATTR)&threadattr,
                                                          LW_NULL);
        if (hCtrl->AHCICTRL_hMonitorThread == LW_OBJECT_HANDLE_INVALID) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d create ahci monitor thread failed.", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bMonitorStarted = LW_TRUE;
    }

    if (hCtrl->AHCICTRL_bInstalled != LW_FALSE) {                       /* 控制器已经被安装             */
        return  (ERROR_NONE);
    }

    /*
     *  初始化控制器
     */
    hCtrl->AHCICTRL_bInstalled = LW_TRUE;
    hCtrl->AHCICTRL_ulSemTimeout = AHCI_SEM_TIMEOUT_DEF;
    hCtrl->AHCICTRL_piConfigType = &_GiAhciConfigType[0];

    iRet = API_AhciCtrlReset(hCtrl);                                    /* 复位控制器                   */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d control reset failed.", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    if (hDrv->AHCIDRV_pfuncVendorCtrlInit) {
        iRet = hDrv->AHCIDRV_pfuncVendorCtrlInit(hCtrl);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d vendor control init failed.", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    if (hCtrl->AHCICTRL_bIntConnect == LW_FALSE) {                      /* 中断未链接                   */
        iRet = API_AhciCtrlIntConnect(hCtrl, __ahciIsr, "ahci_isr");    /* 链接控制器中断               */
        if (iRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ctrl %d control int connect failed.", hCtrl->AHCICTRL_uiIndex);
            return  (PX_ERROR);
        }
        hCtrl->AHCICTRL_bIntConnect = LW_TRUE;                          /* 标识中断已经链接             */
    }

    iRet = API_AhciCtrlAhciModeEnable(hCtrl);                           /* 使能控制器 AHCI 模式         */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d enable ahci mode failed.", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    iRet = API_AhciCtrlSssSet(hCtrl, LW_TRUE);                          /* 使能 Staggered Spin-up       */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %d enable Staggered Spin-up failed.", hCtrl->AHCICTRL_uiIndex);
        return  (PX_ERROR);
    }

    API_AhciCtrlCapGet(hCtrl);                                          /* 获取控制器能力信息           */
    API_AhciCtrlImpPortGet(hCtrl);                                      /* 获取端口参数                 */
    API_AhciCtrlInfoShow(hCtrl);                                        /* 展示控制器详细信息           */

    /*
     *  分配驱动器控制块
     */
    stSizeTemp = sizeof(AHCI_DRIVE_CB) * (size_t)hCtrl->AHCICTRL_uiImpPortNum;
    hCtrl->AHCICTRL_hDrive = (AHCI_DRIVE_HANDLE)__SHEAP_ZALLOC(stSizeTemp);
    if (!hCtrl->AHCICTRL_hDrive) {
        AHCI_LOG(AHCI_LOG_ERR, "alloc drive tcb failed.", 0);
        return  (PX_ERROR);
    }

    /*
     *  构建控制器与驱动器内存结构
     */
    stMemSize = AHCI_CMD_LIST_ALIGN + AHCI_CMD_LIST_ALIGN +
                (sizeof(AHCI_RECV_FIS_CB) * AHCI_RCV_FIS_MAX) +
                (sizeof(AHCI_CMD_TABLE_CB) * (size_t)hCtrl->AHCICTRL_uiCmdSlotNum);
    stMemSize *= hCtrl->AHCICTRL_uiImpPortNum;
    if (!stMemSize) {
        AHCI_LOG(AHCI_LOG_ERR, "imp port number 0.", 0);
        return  (PX_ERROR);
    }
    pucCmdList = (UINT8 *)API_CacheDmaMalloc(stMemSize);                /* 分配物理内存                 */
    if (!pucCmdList) {
        AHCI_LOG(AHCI_LOG_ERR, "alloc dma buf size 0x%08x failed.", stMemSize);
        return  (PX_ERROR);
    }
    hCtrl->AHCICTRL_pvMemory = (PVOID)pucCmdList;                       /* 保存物理内存句柄             */
    hCtrl->AHCICTRL_ulMemorySize = (ULONG)stMemSize;                    /* 保存物理内存大小             */
    lib_bzero(pucCmdList, stMemSize);                                   /* 清零内存区域                 */
    API_CacheDmaFlush((PVOID)pucCmdList, stMemSize);                    /* 回写物理内存                 */

    /*
     *  获取对齐后的物理内存区域
     */
    pucCmdList = (UINT8 *)(((ULONG)pucCmdList + AHCI_CMD_LIST_ALIGN) & ~(AHCI_CMD_LIST_ALIGN - 1));
    hCtrl->AHCICTRL_pucCmdList = pucCmdList;
    pucRecvFis = pucCmdList + hCtrl->AHCICTRL_uiImpPortNum * AHCI_CMD_LIST_ALIGN;
    AHCI_LOG(AHCI_LOG_PRT, "alloc cmd list addr %p size 0x%08x fis addr %p size 0x%08x.",
             pucCmdList, AHCI_CMD_LIST_ALIGN, pucRecvFis, sizeof(AHCI_RECV_FIS_CB) * AHCI_RCV_FIS_MAX);

    AHCI_LOG(AHCI_LOG_PRT, "port num %d active %d map 0x%08x.",
             hCtrl->AHCICTRL_uiPortNum, hCtrl->AHCICTRL_uiImpPortNum, hCtrl->AHCICTRL_uiImpPortMap);
    iCurrPort = 0;                                                      /* 获得当前端口                 */
    uiPortMap = hCtrl->AHCICTRL_uiImpPortMap;                           /* 端口分布信息                 */
    for (i = 0; i < hCtrl->AHCICTRL_uiPortNum; i++) {                   /* 处理各个端口                 */
        if (uiPortMap & 1) {                                            /* 端口有效                     */
            AHCI_LOG(AHCI_LOG_PRT, "port %d current port %d.", i, iCurrPort);

            hDrive = &hCtrl->AHCICTRL_hDrive[iCurrPort];                /* 获取驱动器控制句柄           */
            LW_SPIN_INIT(&hDrive->AHCIDRIVE_slLock);                    /* 初始化驱动器自旋锁           */
            hDrive->AHCIDRIVE_hCtrl  = hCtrl;                           /* 保存控制器句柄               */
            hDrive->AHCIDRIVE_uiPort = i;                               /* 获取当前端口索引             */
                                                                        /* 获取驱动器基地址             */
            hDrive->AHCIDRIVE_pvRegAddr = (PVOID)((ULONG)hCtrl->AHCICTRL_pvRegAddr + AHCI_DRIVE_BASE(i));
            hDrive->AHCIDRIVE_uiIntCount = 0;                           /* 初始化中断计数               */
            hDrive->AHCIDRIVE_uiTimeoutErrorCount = 0;                  /* 初始化超时错误计数           */
            hDrive->AHCIDRIVE_uiTaskFileErrorCount = 0;                 /* 初始化TF 错误计数            */
            hDrive->AHCIDRIVE_iNextTag = 0;                             /* 初始化标记索引               */
            iCurrPort += 1;                                             /* 更新当前端口                 */
            /*
             *  获取驱动器起始扇区
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_SECTOR_START_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_ulStartSector));
            if (iRet != ERROR_NONE) {
                hDrive->AHCIDRIVE_ulStartSector = 0;
            }
            /*
             *  获取驱动器单次探测时间
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_PROB_TIME_UNIT_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_uiProbTimeUnit));
            if ((iRet != ERROR_NONE) ||
                (hDrive->AHCIDRIVE_uiProbTimeUnit == 0)) {
                hDrive->AHCIDRIVE_uiProbTimeUnit = AHCI_DRIVE_PROB_TIME_UNIT;
            }
            /*
             *  获取探测次数
             */
            iRet = hDrv->AHCIDRV_pfuncOptCtrl(hCtrl, 0, AHCI_OPT_CMD_PROB_TIME_COUNT_GET,
                                              (LONG)((ULONG *)&hDrive->AHCIDRIVE_uiProbTimeCount));
            if ((iRet != ERROR_NONE) ||
                (hDrive->AHCIDRIVE_uiProbTimeCount == 0)) {
                hDrive->AHCIDRIVE_uiProbTimeCount = AHCI_DRIVE_PROB_TIME_COUNT;
            }

            if (hDrv->AHCIDRV_pfuncVendorDriveInit) {
                iRet = hDrv->AHCIDRV_pfuncVendorDriveInit(hDrive);
            } else {
                iRet = ERROR_NONE;
            }

            if (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "port %d vendor init failed.", hDrive->AHCIDRIVE_uiPort);
                return  (PX_ERROR);
            }
            /*
             *  更新命令列表地址信息
             */
            hDrive->AHCIDRIVE_hCmdList = (AHCI_CMD_LIST_HANDLE)pucCmdList;
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLB);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCLB, AHCI_ADDR_LOW32(pucCmdList));
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLB);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLBU);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCLBU, AHCI_ADDR_HIGH32(pucCmdList));
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCLBU);
            pucCmdList += AHCI_CMD_LIST_ALIGN;
            /*
             *  更新接收 FIS 地址信息
             */
            hDrive->AHCIDRIVE_hRecvFis = (AHCI_RECV_FIS_HANDLE)pucRecvFis;
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFB);
            AHCI_PORT_WRITE(hDrive, AHCI_PxFB, AHCI_ADDR_LOW32(pucRecvFis));
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFB);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFBU);
            AHCI_PORT_WRITE(hDrive, AHCI_PxFBU, AHCI_ADDR_HIGH32(pucRecvFis));
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxFBU);
            pucRecvFis += sizeof(AHCI_RECV_FIS_CB) * AHCI_RCV_FIS_MAX;
            /*
             *  更新命令命令表地址到命令列表
             */
            hDrive->AHCIDRIVE_hCmdTable = (AHCI_CMD_TABLE_HANDLE)pucRecvFis;
            for (j = 0; j < hCtrl->AHCICTRL_uiCmdSlotNum; j++) {
                hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCmdTableAddrLow =
                                              AHCI_SWAP(AHCI_ADDR_LOW32(&hDrive->AHCIDRIVE_hCmdTable[j]));
                hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCmdTableAddrHigh =
                                             AHCI_SWAP(AHCI_ADDR_HIGH32(&hDrive->AHCIDRIVE_hCmdTable[j]));
                AHCI_LOG(AHCI_LOG_PRT,
                         "cmd list %2d addr %p low 0x%08x high 0x%08x.",
                         j, &hDrive->AHCIDRIVE_hCmdTable[j],
                         hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCmdTableAddrLow,
                         hDrive->AHCIDRIVE_hCmdList[j].AHCICMDLIST_uiCmdTableAddrHigh);
            }
            pucRecvFis += (sizeof(AHCI_CMD_TABLE_CB) * (size_t)hCtrl->AHCICTRL_uiCmdSlotNum);
            API_CacheDmaFlush(hDrive->AHCIDRIVE_hCmdList, AHCI_CMD_LIST_ALIGN);
            
            /*
             *  创建命令槽同步信号量
             */
            for (j = 0; j < hCtrl->AHCICTRL_uiCmdSlotNum; j++) {
                hDrive->AHCIDRIVE_hSyncBSem[j] = API_SemaphoreBCreate("ahci_sync",
                                                                      LW_FALSE,
                                                                      (LW_OPTION_WAIT_FIFO |
                                                                       LW_OPTION_OBJECT_GLOBAL),
                                                                      LW_NULL);
                AHCI_LOG(AHCI_LOG_PRT, "slot %2d sync sem %p.", j, hDrive->AHCIDRIVE_hSyncBSem[j]);
            }

            hDrive->AHCIDRIVE_uiCmdStarted = 0;                         /* 初始化命令起始状态           */
                                                                        /* 初始化队列深度               */
            hDrive->AHCIDRIVE_uiQueueDepth = hCtrl->AHCICTRL_uiCmdSlotNum;
            hDrive->AHCIDRIVE_bQueued = LW_FALSE;                       /* 初始化是否使能队列模式       */
            /*
             *  初始化同步锁
             */
            hDrive->AHCIDRIVE_hLockMSem = API_SemaphoreMCreate("ahci_dlock",
                                                               LW_PRIO_DEF_CEILING,
                                                               (LW_OPTION_WAIT_PRIORITY |
                                                                LW_OPTION_DELETE_SAFE |
                                                                LW_OPTION_INHERIT_PRIORITY |
                                                                LW_OPTION_OBJECT_GLOBAL),
                                                               LW_NULL);
            hDrive->AHCIDRIVE_hTagMuteSem = API_SemaphoreMCreate("ahci_tag",
                                                                 LW_PRIO_DEF_CEILING,
                                                                 (LW_OPTION_WAIT_FIFO |
                                                                  LW_OPTION_DELETE_SAFE |
                                                                  LW_OPTION_INHERIT_PRIORITY |
                                                                  LW_OPTION_OBJECT_GLOBAL),
                                                                 LW_NULL);
            hDrive->AHCIDRIVE_hQueueSlotCSem = API_SemaphoreCCreate("ahci_slot",
                                                                    1,
                                                                    hCtrl->AHCICTRL_uiCmdSlotNum * 2,
                                                                    (LW_OPTION_WAIT_PRIORITY |
                                                                     LW_OPTION_OBJECT_GLOBAL),
                                                                    LW_NULL);
            /*
             *  清除错误
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSERR);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSERR);
            AHCI_PORT_WRITE(hDrive, AHCI_PxSERR, uiReg);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSERR);
            /*
             *  清除中断状态
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIS);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxIS);
            AHCI_PORT_WRITE(hDrive, AHCI_PxIS, uiReg);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIS);
            /*
             *  使能中断
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIE);
            AHCI_PORT_WRITE(hDrive, AHCI_PxIE,
                            AHCI_PIE_PRCE | AHCI_PIE_PCE | AHCI_PIE_PSE | AHCI_PIE_DSE | AHCI_PIE_DPE |
                            AHCI_PIE_DHRE | AHCI_PIS_SDBS);
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxIE);
            /*
             *  复位命令与状态
             */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxCMD);
            if (uiReg & (AHCI_PCMD_CR | AHCI_PCMD_FR | AHCI_PCMD_FRE | AHCI_PCMD_ST)) {
                uiReg &= ~AHCI_PCMD_ST;
                AHCI_PORT_WRITE(hDrive, AHCI_PxCMD, uiReg);
                API_AhciDriveRegWait(hDrive,
                                     AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 20, 50, &uiReg);
            }
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxCMD);
            AHCI_PORT_WRITE(hDrive, AHCI_PxCMD,
                            (AHCI_PCMD_ICC_ACTIVE | AHCI_PCMD_FRE | AHCI_PCMD_CLO | AHCI_PCMD_POD |
                             AHCI_PCMD_SUD));
            API_AhciDriveRegWait(hDrive,
                                 AHCI_PxCMD, AHCI_PCMD_CR, LW_TRUE, AHCI_PCMD_CR, 20, 50, &uiReg);
            /*
             *  等待稳定状态
             */
            iRet = API_AhciDriveRegWait(hDrive,
                                        AHCI_PxTFD, AHCI_STAT_ACCESS, LW_FALSE, 0, 20, 50, &uiReg);
        }

        uiPortMap >>= 1;
    }
    
    /*
     *  使能控制器中断
     */
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_GHC);
    AHCI_CTRL_WRITE(hCtrl, AHCI_GHC, uiReg | AHCI_GHC_IE);
    AHCI_CTRL_REG_MSG(hCtrl, AHCI_GHC);

    for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum ; i++) {               /* 初始化指定驱动器             */
        hDrive = &hCtrl->AHCICTRL_hDrive[i];                            /* 获取驱动器句柄               */
        hDrive->AHCIDRIVE_hDev = LW_NULL;                               /* 初始化设备句柄               */
        hDrive->AHCIDRIVE_ucState = AHCI_DEV_INIT;                      /* 初始化驱动器状态             */
        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_NONE;                      /* 初始化设备类型               */
        hDrive->AHCIDRIVE_bNcq = LW_FALSE;                              /* 初始化 NCQ 标志              */
        hDrive->AHCIDRIVE_bPortError = LW_FALSE;                        /* 初始化端口错误状态           */
        hDrive->AHCIDRIVE_iInitActive = LW_FALSE;                       /* 初始化活动状态               */

        for (j = 0; j < AHCI_RETRY_NUM; j++) {                          /* 探测设备是否已经插入         */
            AHCI_PORT_REG_MSG(hDrive, AHCI_PxSSTS);
            uiReg = AHCI_PORT_READ(hDrive, AHCI_PxSSTS) & AHCI_PSSTS_DET_MSK;
            if (uiReg == AHCI_PSSTS_DET_PHY) {
                break;
            }

            API_TimeMSleep(50);                                         /* 重新等待探测结果             */
        }
        if ((j < AHCI_RETRY_NUM) &&
            (uiReg == AHCI_PSSTS_DET_PHY)) {                            /* 已经探测到设备               */
            AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d phy det.", hCtrl->AHCICTRL_uiIndex, i);
            __ahciDiskCtrlInit(hCtrl, i);                               /* 初始化磁盘控制器             */
            __ahciDiskDriveInit(hCtrl, i);                              /* 初始磁盘驱动器               */
        
        } else {                                                        /* 没有探测到设备               */
            AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d phy not det.", hCtrl->AHCICTRL_uiIndex, i);

            hDrive->AHCIDRIVE_ucType = AHCI_TYPE_NONE;
            hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciNoDataCommandSend
** 功能描述: 发送无数据传输的命令
** 输　入  : hCtrl          控制器句柄
**           uiDrive        驱动器号
**           uiFeature      特定功能
**           usSector       扇区数
**           ucLbaLow       LBA 参数
**           ucLbaMid       LBA 参数
**           ucLbaHigh      LBA 参数
**           iFlags         标记
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciNoDataCommandSend (AHCI_CTRL_HANDLE  hCtrl,
                                UINT              uiDrive,
                                UINT8             ucCmd,
                                UINT32            uiFeature,
                                UINT16            usSector,
                                UINT8             ucLbaLow,
                                UINT8             ucLbaMid,
                                UINT8             ucLbaHigh,
                                INT               iFlags)
{
    INT                 iRet;                                           /* 操作结果                     */
    AHCI_CMD_CB         tCtrlCmd;                                       /* 命令控制块                   */
    AHCI_CMD_HANDLE     hCmd;                                           /* 命令句柄                     */

    hCmd = &tCtrlCmd;                                                   /* 获取命令句柄                 */
    /*
     *  构造命令
     */
    lib_bzero(hCmd, sizeof(AHCI_CMD_CB));
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ucAtaCommand = ucCmd;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaCount = usSector;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_uiAtaFeature = uiFeature;
    hCmd->AHCI_CMD_ATA.AHCICMDATA_ullAtaLba = (UINT64)((ucLbaHigh << 16) | (ucLbaMid << 8) | ucLbaLow);
    hCmd->AHCICMD_iDirection = AHCI_DATA_DIR_NONE;
    hCmd->AHCICMD_iFlags = (INT)(iFlags | AHCI_CMD_FLAG_NON_SEC_DATA);

    iRet = __ahciDiskCommandSend(hCtrl, uiDrive, hCmd);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlFree
** 功能描述: 释放一个 AHCI 控制器
** 输　入  : hCtrl     控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlFree (AHCI_CTRL_HANDLE  hCtrl)
{
    API_AhciCtrlDelete(hCtrl);                                          /* 删除控制器                   */

    if (hCtrl != LW_NULL) {
        __KHEAP_FREE(hCtrl);                                            /* 释放控制器                   */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlCreate
** 功能描述: 创建 AHCI 控制器
** 输　入  : pcName     控制器名称
**           iUnit      本类控制器索引
**           pvArg      扩展参数
** 输　出  : AHCI 控制器句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
AHCI_CTRL_HANDLE  API_AhciCtrlCreate (CPCHAR  pcName, UINT  uiUnit, PVOID  pvArg)
{
    INT                 iRet     = PX_ERROR;                            /* 操作结果                     */
    REGISTER INT        i        = 0;                                   /* 循环因子                     */
    AHCI_CTRL_HANDLE    hCtrl    = LW_NULL;                             /* 控制器句柄                   */
    AHCI_DRV_HANDLE     hDrv     = LW_NULL;                             /* 驱动句柄                     */
    CHAR                cDriveName[AHCI_DEV_NAME_MAX] = {0};            /* 驱动器设备名称               */

    if ((!pcName   ) ||
        (uiUnit < 0)) {                                                 /* 控制器参数错误               */
        return  (LW_NULL);
    }

    hCtrl = (AHCI_CTRL_HANDLE)__SHEAP_ZALLOC(sizeof(AHCI_CTRL_CB));     /* 分配控制器控制块             */
    if (!hCtrl) {                                                       /* 分配控制块失败               */
        AHCI_LOG(AHCI_LOG_ERR, "alloc ctrl %s unit %d tcb failed.", pcName, uiUnit);
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }

    hDrv = API_AhciDrvHandleGet(pcName);                                /* 通过名字获得驱动句柄         */
    if (!hDrv) {                                                        /* 驱动未注册                   */
        AHCI_LOG(AHCI_LOG_ERR, "ahci driver %s not register.", pcName);
        goto  __error_handle;
    }

    hCtrl->AHCICTRL_hDrv        = hDrv;                                 /* 控制器驱动                   */
    lib_strlcpy(&hCtrl->AHCICTRL_cCtrlName[0], &hDrv->AHCIDRV_cDrvName[0], AHCI_CTRL_NAME_MAX);
    hCtrl->AHCICTRL_uiCoreVer   = AHCI_CTRL_DRV_VER_NUM;                /* 控制器核心版本               */
    hCtrl->AHCICTRL_uiUnitIndex = uiUnit;                               /* 本类控制器索引               */
    hCtrl->AHCICTRL_uiIndex     = API_AhciCtrlIndexGet();               /* 控制器索引                   */
    hCtrl->AHCICTRL_pvArg       = pvArg;
    API_AhciCtrlAdd(hCtrl);                                             /* 添加控制器                   */

    if (hDrv->AHCIDRV_pfuncVendorCtrlReadyWork) {
        iRet = hDrv->AHCIDRV_pfuncVendorCtrlReadyWork(hCtrl);
    } else {
        iRet = ERROR_NONE;
    }

    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d vendor ready work failed.", pcName, uiUnit);
        goto  __error_handle;
    }

    if (!hCtrl->AHCICTRL_pvRegAddr) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d reg addr null.", 0);
        goto  __error_handle;
    }

    iRet = __ahciDrvInit(hCtrl);                                        /* 驱动初始化                   */
    if (iRet != ERROR_NONE) {
        AHCI_LOG(AHCI_LOG_ERR, "ctrl %s unit %d driver init failed.", pcName, uiUnit);
        goto  __error_handle;
    }
    for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum; i++) {                /* 驱动器初始化                 */
        snprintf(cDriveName, AHCI_DEV_NAME_MAX, "/" AHCI_NAME "c%dd%d", hCtrl->AHCICTRL_uiIndex, i);
        __ahciDiskConfig(hCtrl, i, &cDriveName[0]);                     /* 初始化指定驱动器             */
    }

    return  (hCtrl);                                                    /* 返回控制器句柄               */

__error_handle:                                                         /* 错误处理                     */
    API_AhciCtrlFree(hCtrl);                                            /* 释放控制器                   */

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __ahciMonitorThread
** 功能描述: AHCI 监测线程
** 输　入  : pvArg     线程参数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __ahciMonitorThread (PVOID  pvArg)
{
    ULONG               ulRet;                                          /* 操作结果                     */
    AHCI_MSG_CB         tCtrlMsg;                                       /* 消息控制块                   */
    size_t              stTemp = 0;                                     /* 临时大小                     */
    INT                 iMsgId;                                         /* 消息标识                     */
    INT                 iDrive;                                         /* 驱动器索引                   */
    AHCI_DRIVE_HANDLE   hDrive;                                         /* 驱动器句柄                   */
    AHCI_CTRL_HANDLE    hCtrl;                                          /* 控制器句柄                   */
    PLW_BLK_DEV         hBlkDev = LW_NULL;                              /* 块设备句柄                   */
    INT                 iRetry;                                         /* 重试参数                     */
    INT                 iRet;                                           /* 操作结果                     */

    for (;;) {
        hCtrl = (AHCI_CTRL_HANDLE)pvArg;                                /* 获取控制器句柄               */
        ulRet = API_MsgQueueReceive(hCtrl->AHCICTRL_hMsgQueue,
                                    (PVOID)&tCtrlMsg,
                                    AHCI_MSG_SIZE,
                                    &stTemp,
                                    LW_OPTION_WAIT_INFINITE);           /* 等待消息                     */
        if (ulRet != ERROR_NONE) {
            AHCI_LOG(AHCI_LOG_ERR, "ahci msg queue recv error.", 0);
            continue;                                                   /* 接收到错误消息后继续等待     */
        }

        if (hCtrl != tCtrlMsg.AHCIMSG_hCtrl) {                          /* 控制器句柄无效               */
            AHCI_LOG(AHCI_LOG_ERR,"msg handle error.", 0);
            continue;
        }
        hCtrl = tCtrlMsg.AHCIMSG_hCtrl;
        iDrive = tCtrlMsg.AHCIMSG_uiDrive;
        if ((iDrive < 0) ||
            (iDrive >= hCtrl->AHCICTRL_uiImpPortNum)) {                 /* 驱动器索引错误               */
            AHCI_LOG(AHCI_LOG_ERR,"drive %d is out of range (0-%d).", iDrive, (AHCI_DRIVE_MAX - 1));
            continue;
        }
        iMsgId = tCtrlMsg.AHCIMSG_uiMsgId;
        hDrive = &hCtrl->AHCICTRL_hDrive[iDrive];
        /*
         *  处理指定消息
         */
        switch (iMsgId) {

        case AHCI_MSG_ATTACH:                                           /* 设备接入                     */
            AHCI_LOG(AHCI_LOG_PRT,"recv attach msg ctrl %d drive %d.", hCtrl->AHCICTRL_uiIndex, iDrive);
            if (hDrive->AHCIDRIVE_ucState != AHCI_DEV_NONE) {
                AHCI_LOG(AHCI_LOG_PRT, "ctrl %d drive %d state none.", hCtrl->AHCICTRL_uiIndex, iDrive);
                continue;
            }

            hDrive->AHCIDRIVE_ucState = AHCI_DEV_INIT;
            AHCI_LOG(AHCI_LOG_PRT, "init ctrl %d drive %d.", hCtrl->AHCICTRL_uiIndex, iDrive);

            iRet = __ahciDriveNoBusyWait(hDrive);
            if (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR,
                         "wait ctrl %d drive %d no busy failed time %d ms.",
                         hCtrl->AHCICTRL_uiIndex, iDrive,
                         (hDrive->AHCIDRIVE_uiProbTimeUnit * hDrive->AHCIDRIVE_uiProbTimeCount));
                break;
            }

            iRetry = 0;
            iRet = __ahciDiskCtrlInit(hCtrl, iDrive);
            while (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "ctrl init err ctrl %d drive %d retry %d.",
                         hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                iRetry += 1;
                if (iRetry >= AHCI_RETRY_NUM) {
                    break;
                }
                iRet = __ahciDiskCtrlInit(hCtrl, iDrive);
            }

            iRetry = 0;
            iRet = __ahciDiskDriveInit(hCtrl, iDrive);
            while (iRet != ERROR_NONE) {
                AHCI_LOG(AHCI_LOG_ERR, "drive init err ctrl %d drive %d retry %d.",
                         hCtrl->AHCICTRL_uiIndex, iDrive, iRetry);
                iRetry += 1;
                if (iRetry >= AHCI_RETRY_NUM) {
                    break;
                }

                iRet = __ahciDiskDriveInit(hCtrl, iDrive);
            }

            hBlkDev = __ahciBlkDevCreate(hCtrl, iDrive, hDrive->AHCIDRIVE_ulStartSector, 0);
            if (!hBlkDev) {
                AHCI_LOG(AHCI_LOG_ERR, "create blk dev error %s", hDrive->AHCIDRIVE_cDevName);
                break;
            }
            API_AhciDevAdd(hCtrl, iDrive);
            break;

        case AHCI_MSG_REMOVE:                                           /* 设备移除                     */
            AHCI_LOG(AHCI_LOG_PRT, "remove ctrl %d drive %d.", hCtrl->AHCICTRL_uiIndex, iDrive);
            if (hDrive->AHCIDRIVE_hDev != LW_NULL) {
                __ahciBlkDevRemove(hCtrl, iDrive);
            }
            break;

        case AHCI_MSG_ERROR:                                            /* 设备错误                     */
            AHCI_LOG(AHCI_LOG_ERR, "error ctrl %d drive %d.", hCtrl->AHCICTRL_uiIndex, iDrive);
            __ahciDiskCtrlInit(hCtrl, iDrive);
            hDrive->AHCIDRIVE_bPortError = LW_FALSE;
            break;

        default:
            break;
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __ahciIsr
** 功能描述: 中断服务
** 输　入  : hCtrl    控制器句柄
**           ulVector       中断向量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t __ahciIsr (PVOID  pvArg, ULONG  ulVector)
{
    AHCI_CTRL_HANDLE    hCtrl = (AHCI_CTRL_HANDLE)pvArg;
    REGISTER INT        i;
    REGISTER INT        j;
    AHCI_DRIVE_HANDLE   hDrive;
    AHCI_MSG_CB         tCtrlMsg;
    UINT32              uiCtrlIntr;
    UINT32              uiSataIntr;
    UINT32              uiPortIntr;
    UINT32              uiTaskStatus;
    UINT32              uiSataStatus;
    UINT32              uiCmdActive;
    UINT32              uiSataActive;
    UINT32              uiActive;
    UINT32              uiReg;
    UINT32              uiSlotBit;

    tCtrlMsg.AHCIMSG_hCtrl = hCtrl;
    uiReg = AHCI_CTRL_READ(hCtrl, AHCI_IS);
    if (!uiReg) {
        return  (LW_IRQ_NONE);
    }

    while (uiReg) {
        for (i = 0; i < hCtrl->AHCICTRL_uiImpPortNum; i++) {
            hDrive = &hCtrl->AHCICTRL_hDrive[i];
            uiCtrlIntr = AHCI_CTRL_READ(hCtrl, AHCI_IS) & (_GuiAhciBitMask[hDrive->AHCIDRIVE_uiPort]);
            if (uiCtrlIntr & (_GuiAhciBitMask[hDrive->AHCIDRIVE_uiPort])) {
                hDrive->AHCIDRIVE_uiIntCount += 1;

                uiSataIntr = AHCI_PORT_READ(hDrive, AHCI_PxSERR);
                uiPortIntr = AHCI_PORT_READ(hDrive, AHCI_PxIS);
                AHCI_INT_LOG(AHCI_LOG_PRT, "AHCI_PxSERR 0x%08x.", uiSataIntr);
                AHCI_INT_LOG(AHCI_LOG_PRT, "AHCI_PxIS 0x%08x.", uiPortIntr);

                AHCI_PORT_WRITE(hDrive, AHCI_PxSERR, uiSataIntr);
                AHCI_PORT_WRITE(hDrive, AHCI_PxIS, uiPortIntr);
                AHCI_CTRL_WRITE(hCtrl, AHCI_IS, uiCtrlIntr);

                uiTaskStatus = AHCI_PORT_READ(hDrive, AHCI_PxTFD);
                uiSataStatus = AHCI_PORT_READ(hDrive, AHCI_PxSSTS);
                hDrive->AHCIDRIVE_uiIntStatus = uiTaskStatus & 0xFF;
                hDrive->AHCIDRIVE_uiIntError  = (uiTaskStatus & 0xFF00) >> 8;
                uiCmdActive = AHCI_PORT_READ(hDrive, AHCI_PxCI);
                uiSataActive = AHCI_PORT_READ(hDrive, AHCI_PxSACT);
                AHCI_INT_LOG(AHCI_LOG_PRT,
                             "AHCI_PxTFD 0x%08x AHCI_PxSSTS 0x%08x AHCI_PxCI 0x%08x AHCI_PxSACT 0x%08x.",
                             uiTaskStatus, uiSataStatus, uiCmdActive, uiSataActive);

                if ((uiPortIntr & AHCI_PIS_PRCS) &&
                    ((uiSataStatus & AHCI_PSSTS_IPM_MSK) == AHCI_PSSTS_IPM_ACTIVE)) {
                    if (hDrive->AHCIDRIVE_iInitActive == LW_FALSE) {
                        AHCI_INT_LOG(AHCI_LOG_PRT, "insert ctrl %d port %d.",
                                     hCtrl->AHCICTRL_uiIndex,hDrive->AHCIDRIVE_uiPort);

                        tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_ATTACH;
                        tCtrlMsg.AHCIMSG_uiDrive = i;
                        API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                        continue;
                    }
                }

                if ((uiPortIntr & AHCI_PIS_PRCS) &&
                    ((uiSataStatus & AHCI_PSSTS_IPM_MSK) == AHCI_PSSTS_IPM_DEVICE_NONE)) {
                    if (hDrive->AHCIDRIVE_iInitActive == LW_FALSE) {
                        AHCI_INT_LOG(AHCI_LOG_PRT, "remove ctrl %d port %d.",
                                     hCtrl->AHCICTRL_uiIndex,hDrive->AHCIDRIVE_uiPort);

                        hDrive->AHCIDRIVE_ucType = AHCI_TYPE_NONE;
                        hDrive->AHCIDRIVE_ucState = AHCI_DEV_NONE;
                        hDrive->AHCIDRIVE_hDev->AHCIDEV_tBlkDev.BLKD_bDiskChange = LW_TRUE;
                        for (j = 0; j < hDrive->AHCIDRIVE_uiQueueDepth; j++) {
                            if (hDrive->AHCIDRIVE_uiCmdStarted & (_GuiAhciBitMask[j])) {
                                API_SemaphoreBPost(hDrive->AHCIDRIVE_hSyncBSem[j]);
                            }
                        }
                        tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_REMOVE;
                        tCtrlMsg.AHCIMSG_uiDrive = i;
                        API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                    }
                    continue;
                }

                if (uiPortIntr & AHCI_PIS_TFES) {
                    AHCI_INT_LOG(AHCI_LOG_PRT, "Task File Error ctrl %d port %d deep %d.",
                                 hCtrl->AHCICTRL_uiIndex, hDrive->AHCIDRIVE_uiPort,
                                 hDrive->AHCIDRIVE_uiQueueDepth);

                    hDrive->AHCIDRIVE_bPortError = LW_TRUE;
                    hDrive->AHCIDRIVE_uiTaskFileErrorCount++;
                    for (j = 0; j < hDrive->AHCIDRIVE_uiQueueDepth; j++) {
                        if (hDrive->AHCIDRIVE_uiCmdStarted & (_GuiAhciBitMask[j])) {
                            API_SemaphoreBPost(hDrive->AHCIDRIVE_hSyncBSem[j]);
                        }
                    }

                    hDrive->AHCIDRIVE_uiCmdStarted = 0;

                    tCtrlMsg.AHCIMSG_uiMsgId = AHCI_MSG_ERROR;
                    tCtrlMsg.AHCIMSG_uiDrive = i;
                    API_MsgQueueSend(hCtrl->AHCICTRL_hMsgQueue, (PVOID)&tCtrlMsg, AHCI_MSG_SIZE);
                    continue;
                }

                if (hDrive->AHCIDRIVE_bQueued == LW_TRUE) {
                    uiActive = uiSataActive;
                } else {
                    uiActive = uiCmdActive;
                }

                for (j = 0; j < hDrive->AHCIDRIVE_uiQueueDepth; j++) {
                    uiSlotBit = _GuiAhciBitMask[j];
                    if (hDrive->AHCIDRIVE_uiCmdStarted & uiSlotBit) {
                        if (!(uiActive & uiSlotBit)) {
                            API_SemaphoreBPost(hDrive->AHCIDRIVE_hSyncBSem[j]);
                            hDrive->AHCIDRIVE_uiCmdStarted &= ~uiSlotBit;
                        }
                    }
                }
            }
        }

        uiReg = AHCI_CTRL_READ(hCtrl, AHCI_IS);
    }

    return  (LW_IRQ_HANDLED);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
