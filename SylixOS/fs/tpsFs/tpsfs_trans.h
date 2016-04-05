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
** ��   ��   ��: tpsfs_trans.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: ��������

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_TRANS_H
#define __TPSFS_TRANS_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  ������������
*********************************************************************************************************/
typedef struct tps_trans_data {
    UINT                TD_uiSecAreaCnt;
    struct {
        UINT64          TD_ui64SecStart;
        UINT64          TD_ui64SecCnt;
    } TD_secareaArr[1];
} TPS_TRANS_DATA;
typedef TPS_TRANS_DATA  *PTPS_TRANS_DATA;

/*********************************************************************************************************
  ����ṹ
*********************************************************************************************************/

typedef struct tps_trans {
    UINT                 TRANS_uiMagic;
    UINT                 TRANS_uiGeneration;
    UINT64               TRANS_ui64SerialNum;
    INT                  TRANS_iType;
    INT                  TRANS_iStatus;                                 /* ����״̬                     */
    UINT64               TRANS_ui64Time;                                /* �޸�ʱ��                     */
    UINT64               TRANS_uiDataSec;
    UINT                 TRANS_uiDataLen;
    UINT64               TRANS_ui64Reserved[2];                         /* ����2                        */
    UINT                 TRANS_uiCheckSum;

    struct tps_trans    *TRANS_pnext;                                   /* �����б�ָ��                 */
    PTPS_SUPER_BLOCK     TRANS_psb;                                     /* ������ָ��                   */
    PTPS_TRANS_DATA      TRANS_pdata;
    PUCHAR               TRANS_pucSecData;
} TPS_TRANS;
typedef TPS_TRANS       *PTPS_TRANS;

/*********************************************************************************************************
  �������
*********************************************************************************************************/
                                                                        /* �������������               */
TPS_RESULT tspFsCheckTrans(PTPS_SUPER_BLOCK psb);
                                                                        /* �������Ϊһ��״̬           */
TPS_RESULT tspFsCompleteTrans(PTPS_SUPER_BLOCK psb);
                                                                        /* ��������                     */
PTPS_TRANS tpsFsTransAllocAndInit(PTPS_SUPER_BLOCK psb);
                                                                        /* �ع�����                     */
TPS_RESULT tpsFsTransRollBackAndFree(PTPS_TRANS ptrans);
                                                                        /* �ύ����                     */
TPS_RESULT tpsFsTransCommitAndFree(PTPS_TRANS ptrans);
                                                                        /* д�����ݵ�����               */
TPS_RESULT tpsFsTransWrite(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb,
                           TPS_IBLK blk, UINT uiOff,
                           PUCHAR pucBuff, size_t szLen);

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_TRANS_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
