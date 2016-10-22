/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: fsCommon.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 07 �� 05 ��
**
** ��        ��: �ļ�ϵͳ�ӿڹ������񲿷�(������ּ��������, ���ºܶ��ļ�ϵͳ������Ϊû�кܺõķ�װ
                                          ���Ժ�İ汾��, ���������Ľ�).
** BUG:
2009.10.28  Linkcounter ʹ�� atomic ����.
2009.12.01  �������ļ�ϵͳע�ắ��. (������ yaffs �ļ�ϵͳ)
2011.03.06  ���� gcc 4.5.1 ��� warning.
2012.03.08  ���� __LW_FILE_ERROR_NAME_STR �ַ���.
2012.04.01  ����Ȩ���ж�.
2013.01.31  ��Ȩ���жϼ��� IO ϵͳ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
#include "limits.h"
/*********************************************************************************************************
  �ļ������ܰ������ַ� (����ϵͳ�����û����� __LW_FILE_ERROR_NAME_STR, ���ǲ��Ƽ�)
*********************************************************************************************************/
#ifndef __LW_FILE_ERROR_NAME_STR
#define __LW_FILE_ERROR_NAME_STR        "\\*?<>:\"|\t\r\n"              /*  ���ܰ������ļ��ڵ��ַ�      */
#endif                                                                  /*  __LW_FILE_ERROR_NAME_STR    */
/*********************************************************************************************************
  �ļ�ϵͳ����Ӧ���ļ�ϵͳװ�غ��� (����� yaffs ϵͳ)
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                 FSN_lineManage;                        /*  ��������                    */
    FUNCPTR                      FSN_pfuncCreate;                       /*  �ļ�ϵͳ��������            */
    FUNCPTR                      FSN_pfuncCheck;                        /*  �ļ�ϵͳ��麯��            */
    CHAR                         FSN_pcFsName[1];                       /*  �ļ�ϵͳ����                */
} __LW_FILE_SYSTEM_NODE;
typedef __LW_FILE_SYSTEM_NODE   *__PLW_FILE_SYSTEM_NODE;

static LW_LIST_LINE_HEADER       _G_plineFsNodeHeader = LW_NULL;        /*  �ļ�ϵͳ��ڱ�              */
/*********************************************************************************************************
** ��������: __fsRegister
** ��������: ע��һ���ļ�ϵͳ
** �䡡��  : pcName           �ļ�ϵͳ��
**           pfuncCreate      �ļ�ϵͳ��������
**           pfuncCheck       �ļ�ϵͳ��麯��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __fsRegister (CPCHAR  pcName, FUNCPTR  pfuncCreate, FUNCPTR  pfuncCheck)
{
    __PLW_FILE_SYSTEM_NODE      pfsnNew;

    if (!pcName || !pfuncCreate) {
        return  (PX_ERROR);
    }
    
    pfsnNew = (__PLW_FILE_SYSTEM_NODE)__SHEAP_ALLOC(lib_strlen(pcName) + 
                                                    sizeof(__LW_FILE_SYSTEM_NODE));
    if (pfsnNew == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    pfsnNew->FSN_pfuncCreate = pfuncCreate;
    pfsnNew->FSN_pfuncCheck  = pfuncCheck;
    lib_strcpy(pfsnNew->FSN_pcFsName, pcName);
    
    _IosLock();
    _List_Line_Add_Ahead(&pfsnNew->FSN_lineManage, &_G_plineFsNodeHeader);
    _IosUnlock();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fsCreateFuncGet
** ��������: ��ȡ�ļ�ϵͳ��������
** �䡡��  : pcName           �ļ�ϵͳ��
**           pblkd            ��Ӧ����
**           ucPartType       ��Ӧ��������
** �䡡��  : �ļ�ϵͳ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
FUNCPTR  __fsCreateFuncGet (CPCHAR   pcName, PLW_BLK_DEV  pblkd, UINT8  ucPartType)
{
    __PLW_FILE_SYSTEM_NODE      pfsnFind = LW_NULL;
    PLW_LIST_LINE               plineTemp;

    if (!pcName) {
        return  (LW_NULL);
    }
    
    _IosLock();
    for (plineTemp  = _G_plineFsNodeHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pfsnFind = (__PLW_FILE_SYSTEM_NODE)plineTemp;
        if (lib_strcmp(pfsnFind->FSN_pcFsName, pcName) == 0) {
            break;
        }
    }
    _IosUnlock();
    
    if (plineTemp) {
        if (pfsnFind->FSN_pfuncCheck && pblkd) {
            if (pfsnFind->FSN_pfuncCheck(pblkd, ucPartType) < ERROR_NONE) {
                return  (LW_NULL);
            }
        }
        return  (pfsnFind->FSN_pfuncCreate);
    
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __fsCheckFileName
** ��������: ����ļ�������
** �䡡��  : pcName           �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __fsCheckFileName (CPCHAR  pcName)
{
    REGISTER PCHAR  pcTemp;
    
    /*
     *  ���ܽ��� . �� .. �ļ�
     */
    pcTemp = lib_rindex(pcName, PX_DIVIDER);
    if (pcTemp) {
        pcTemp++;
        if (*pcTemp == PX_EOS) {                                        /*  �ļ�������Ϊ 0              */
            return  (PX_ERROR);
        }
        if ((lib_strcmp(pcTemp, ".")  == 0) ||
            (lib_strcmp(pcTemp, "..") == 0)) {                          /*  . , .. ���                 */
            return  (PX_ERROR);
        }
    } else {
        if (pcName[0] == PX_EOS) {                                      /*  �ļ�������Ϊ 0              */
            return  (PX_ERROR);
        }
    }
    
    /*
     *  ���ܰ����Ƿ��ַ�
     */
    pcTemp = (PCHAR)pcName;
    for (; *pcTemp != PX_EOS; pcTemp++) {
        if (lib_strchr(__LW_FILE_ERROR_NAME_STR, *pcTemp)) {            /*  ���Ϸ���                  */
            return  (PX_ERROR);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fsDiskLinkCounterAdd
** ��������: �������������������1
** �䡡��  : pblkd           ���豸���ƿ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __fsDiskLinkCounterAdd (PLW_BLK_DEV  pblkd)
{
    INTREG       iregInterLevel;
    PLW_BLK_DEV  pblkdPhy;

    __LW_ATOMIC_LOCK(iregInterLevel);
    if (pblkd->BLKD_pvLink) {
        pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;                     /*  ��������豸����            */
        pblkdPhy->BLKD_uiLinkCounter++;
        if (pblkdPhy->BLKD_uiLinkCounter == 1) {
            pblkdPhy->BLKD_uiPowerCounter = 0;
            pblkdPhy->BLKD_uiInitCounter  = 0;
        }
    } else {
        pblkd->BLKD_uiLinkCounter++;
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);
}
/*********************************************************************************************************
** ��������: __fsDiskLinkCounterDec
** ��������: �������������������1
** �䡡��  : pblkd           ���豸���ƿ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __fsDiskLinkCounterDec (PLW_BLK_DEV  pblkd)
{
    INTREG       iregInterLevel;
    PLW_BLK_DEV  pblkdPhy;

    __LW_ATOMIC_LOCK(iregInterLevel);
    if (pblkd->BLKD_pvLink) {
        pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;                     /*  ��������豸����            */
        pblkdPhy->BLKD_uiLinkCounter--;
    } else {
        pblkd->BLKD_uiLinkCounter--;
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);
}
/*********************************************************************************************************
** ��������: __fsDiskLinkCounterGet
** ��������: ��ȡ���������������
** �䡡��  : pblkd           ���豸���ƿ�
** �䡡��  : ���������������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT  __fsDiskLinkCounterGet (PLW_BLK_DEV  pblkd)
{
    INTREG       iregInterLevel;
    PLW_BLK_DEV  pblkdPhy;
    UINT         uiRet;

    __LW_ATOMIC_LOCK(iregInterLevel);
    if (pblkd->BLKD_pvLink) {
        pblkdPhy = (PLW_BLK_DEV)pblkd->BLKD_pvLink;                     /*  ��������豸����            */
        uiRet    = pblkdPhy->BLKD_uiLinkCounter;
    } else {
        uiRet    = pblkd->BLKD_uiLinkCounter;
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);

    return  (uiRet);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
