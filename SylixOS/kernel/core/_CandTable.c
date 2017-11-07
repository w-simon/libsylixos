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
** ��   ��   ��: _CandTable.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 12 �� 05 ��
**
** ��        ��: ����ϵͳ�ں˺�ѡ���б������(�� k_sched.h �е� inline �������󵽴�)

** BUG:
2013.07.29  ���ļ�����Ϊ _CandTable.c ��ʾ��ѡ���б����.
2014.01.10  ����ѡ����� CPU �ṹ��.
2015.04.22  ���� _CandTableResel() ��������ٶ�.
2017.10.31  �� _CandTableUpdate() ��һ�������Ƴ�һ������ʱ, �������û�а� CPU, ��Ӧ�������� CPU �ϳ���
            ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _CandSeekThread
** ��������: �Ӿ�������ȷ��һ���������е��߳�.
** �䡡��  : ppcbbmap          ������
**           ucPriority        ���ȼ�
** �䡡��  : �ھ�����������Ҫ���е��߳�.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_CLASS_TCB  _CandSeekThread (PLW_CLASS_PCBBMAP  ppcbbmap, UINT8  ucPriority)
{
    REGISTER PLW_CLASS_PCB      ppcb;
    REGISTER PLW_CLASS_TCB      ptcb;
    
    ppcb = &ppcbbmap->PCBM_pcb[ucPriority];
    ptcb = _LIST_ENTRY(ppcb->PCB_pringReadyHeader, 
                       LW_CLASS_TCB, 
                       TCB_ringReady);                                  /*  �Ӿ�������ȡ��һ���߳�      */
    
    if (ptcb->TCB_ucSchedPolicy == LW_OPTION_SCHED_FIFO) {              /*  ����� FIFO ֱ������        */
        return  (ptcb);
    
    } else if (ptcb->TCB_usSchedCounter == 0) {                         /*  ȱ��ʱ��Ƭ                  */
        ptcb->TCB_usSchedCounter = ptcb->TCB_usSchedSlice;              /*  ����ʱ��Ƭ                  */
        _list_ring_next(&ppcb->PCB_pringReadyHeader);                   /*  ��һ��                      */
        ptcb = _LIST_ENTRY(ppcb->PCB_pringReadyHeader, 
                           LW_CLASS_TCB, 
                           TCB_ringReady);
    }
    
    return  (ptcb);
}
/*********************************************************************************************************
** ��������: _CandTableFill
** ��������: ѡ��һ�����ִ���̷߳����ѡ��.
** �䡡��  : pcpu          CPU �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CandTableFill (PLW_CLASS_CPU   pcpu)
{
    REGISTER PLW_CLASS_TCB      ptcb;
    REGISTER PLW_CLASS_PCB      ppcb;
    REGISTER PLW_CLASS_PCBBMAP  ppcbbmap;
             UINT8              ucPriority;

    ppcbbmap = _SchedSeekPriority(pcpu, &ucPriority);
    _BugHandle((ppcbbmap == LW_NULL), LW_TRUE, "serious error!\r\n");   /*  ������Ӧ��Ϊ NULL         */
    
    ptcb = _CandSeekThread(ppcbbmap, ucPriority);                       /*  ȷ�����Ժ�ѡ���е��߳�      */
    ppcb = &ppcbbmap->PCBM_pcb[ucPriority];
    
    LW_CAND_TCB(pcpu) = ptcb;                                           /*  �����µĿ�ִ���߳�          */
    ptcb->TCB_ulCPUId = pcpu->CPU_ulCPUId;                              /*  ��¼ CPU ��                 */
    ptcb->TCB_bIsCand = LW_TRUE;                                        /*  �����ѡ���б�              */
    _DelTCBFromReadyRing(ptcb, ppcb);                                   /*  �Ӿ��������˳�              */
    
    if (_PcbIsEmpty(ppcb)) {
        __DEL_RDY_MAP(ptcb);                                            /*  �Ӿ�������ɾ��              */
    }
}
/*********************************************************************************************************
** ��������: _CandTableResel
** ��������: ѡ��һ�����ִ���̷߳����ѡ��.
** �䡡��  : pcpu          CPU �ṹ
**           ppcbbmap      ���ȼ�λͼ
**           ucPriority    ��Ҫ������������ȼ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CandTableResel (PLW_CLASS_CPU   pcpu, PLW_CLASS_PCBBMAP  ppcbbmap, UINT8  ucPriority)
{
    REGISTER PLW_CLASS_TCB      ptcb;
    REGISTER PLW_CLASS_PCB      ppcb;
    
    ptcb = _CandSeekThread(ppcbbmap, ucPriority);                       /*  ȷ�����Ժ�ѡ���е��߳�      */
    ppcb = &ppcbbmap->PCBM_pcb[ucPriority];
    
    LW_CAND_TCB(pcpu) = ptcb;                                           /*  �����µĿ�ִ���߳�          */
    ptcb->TCB_ulCPUId = pcpu->CPU_ulCPUId;                              /*  ��¼ CPU ��                 */
    ptcb->TCB_bIsCand = LW_TRUE;                                        /*  �����ѡ���б�              */
    _DelTCBFromReadyRing(ptcb, ppcb);                                   /*  �Ӿ��������˳�              */
    
    if (_PcbIsEmpty(ppcb)) {
        __DEL_RDY_MAP(ptcb);                                            /*  �Ӿ�������ɾ��              */
    }
}
/*********************************************************************************************************
** ��������: _CandTableEmpty
** ��������: ����ѡ���еĺ�ѡ�̷߳��������.
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CandTableEmpty (PLW_CLASS_CPU   pcpu)
{
    REGISTER PLW_CLASS_TCB  ptcb;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    ptcb = LW_CAND_TCB(pcpu);
    ppcb = _GetPcb(ptcb);
    
    ptcb->TCB_bIsCand = LW_FALSE;
    _AddTCBToReadyRing(ptcb, ppcb, LW_TRUE);                            /*  ���¼��������              */
                                                                        /*  ���������ͷ, �´����ȵ���  */
    if (_PcbIsOne(ppcb)) {
        __ADD_RDY_MAP(ptcb);                                            /*  ��ǰ�̱߳���ռ, �ػؾ�����  */
    }
    
    LW_CAND_TCB(pcpu) = LW_NULL;
}
/*********************************************************************************************************
** ��������: _CandTableTryAdd
** ��������: ��ͼ��һ�������߳�װ���ѡ�̱߳�.
** �䡡��  : ptcb          �������߳�
**           ppcb          ��Ӧ�����ȼ����ƿ�
** �䡡��  : �Ƿ�����˺�ѡ���б�
** ȫ�ֱ���: 
** ����ģ��: 
** ˵  ��  : ����ϵͳ�ɲ���Ҫʹ�����Ʊ�־, ����������ѡ��.
*********************************************************************************************************/
BOOL  _CandTableTryAdd (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb)
{
    REGISTER PLW_CLASS_CPU   pcpu;
    REGISTER PLW_CLASS_TCB   ptcbCand;
    
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ���                    */
    REGISTER ULONG           i;

    if (ptcb->TCB_bCPULock) {                                           /*  �������� CPU                */
        pcpu = LW_CPU_GET(ptcb->TCB_ulCPULock);
        if (!LW_CPU_IS_ACTIVE(pcpu)) {
            goto    __can_not_cand;
        }
        
        ptcbCand = LW_CAND_TCB(pcpu);
        if (ptcbCand == LW_NULL) {                                      /*  ��ѡ��Ϊ��                  */
            LW_CAND_TCB(pcpu) = ptcb;
            ptcb->TCB_ulCPUId = ptcb->TCB_ulCPULock;                    /*  ��¼ CPU ��                 */
            ptcb->TCB_bIsCand = LW_TRUE;                                /*  �����ѡ���б�              */
            return  (LW_TRUE);
            
        } else if (LW_PRIO_IS_HIGH(ptcb->TCB_ucPriority, 
                                   ptcbCand->TCB_ucPriority)) {         /*  ���ȼ����ڵ�ǰ��ѡ�߳�      */
            LW_CAND_ROT(pcpu) = LW_TRUE;                                /*  �������ȼ�����              */
        }
    
    } else {                                                            /*  ���� CPU �������д�����     */
        LW_CPU_FOREACH_ACTIVE (i) {                                     /*  CPU ����Ϊ����״̬          */
            pcpu     = LW_CPU_GET(i);
            ptcbCand = LW_CAND_TCB(pcpu);                               /*  TODO: Cache �ȶ�ά��        */
            if (ptcbCand == LW_NULL) {                                  /*  ��ѡ��Ϊ��                  */
                LW_CAND_TCB(pcpu) = ptcb;
                ptcb->TCB_ulCPUId = i;                                  /*  ��¼ CPU ��                 */
                ptcb->TCB_bIsCand = LW_TRUE;                            /*  �����ѡ���б�              */
                return  (LW_TRUE);
            
            } else if (LW_PRIO_IS_HIGH(ptcb->TCB_ucPriority, 
                                       ptcbCand->TCB_ucPriority)) {     /*  ���ȼ����ڵ�ǰ��ѡ�߳�      */
                LW_CAND_ROT(pcpu) = LW_TRUE;                            /*  �������ȼ�����              */
            }
        }
    }
    
#else                                                                   /*  �������                    */
    pcpu = LW_CPU_GET(0);
    if (!LW_CPU_IS_ACTIVE(pcpu)) {                                      /*  CPU ����Ϊ����״̬          */
        goto    __can_not_cand;
    }
    
    ptcbCand = LW_CAND_TCB(pcpu);
    if (ptcbCand == LW_NULL) {                                          /*  ��ѡ��Ϊ��                  */
__can_cand:
        LW_CAND_TCB(pcpu) = ptcb;
        ptcb->TCB_ulCPUId = 0;                                          /*  ��¼ CPU ��                 */
        ptcb->TCB_bIsCand = LW_TRUE;                                    /*  �����ѡ���б�              */
        return  (LW_TRUE);                                              /*  ֱ�Ӽ����ѡ���б�          */
        
    } else if (LW_PRIO_IS_HIGH(ptcb->TCB_ucPriority, 
                               ptcbCand->TCB_ucPriority)) {             /*  ���ȼ��ȵ�ǰ��ѡ�̸߳�      */
        if (__THREAD_LOCK_GET(ptcbCand)) {
            LW_CAND_ROT(pcpu) = LW_TRUE;                                /*  �������ȼ�����              */
        
        } else {
            _CandTableEmpty(pcpu);                                      /*  ��պ�ѡ��                  */
            goto    __can_cand;
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */

__can_not_cand:
    if (_PcbIsEmpty(ppcb)) {
        __ADD_RDY_MAP(ptcb);                                            /*  ��λͼ�����λ��һ          */
    }
    
    return  (LW_FALSE);                                                 /*  �޷������ѡ���б�          */
}
/*********************************************************************************************************
** ��������: _CandTableTryDel
** ��������: ��ͼ�ӽ�һ�����پ����ĺ�ѡ�̴߳Ӻ�ѡ�����˳�
** �䡡��  : ptcb          �������߳�
**           ppcb          ��Ӧ�����ȼ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID _CandTableTryDel (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb)
{
    REGISTER PLW_CLASS_CPU   pcpu = LW_CPU_GET(ptcb->TCB_ulCPUId);
    
    if (LW_CAND_TCB(pcpu) == ptcb) {                                    /*  �ں�ѡ����                  */
        ptcb->TCB_bIsCand = LW_FALSE;                                   /*  �˳���ѡ���б�              */
        _CandTableFill(pcpu);                                           /*  �������һ����ѡ�߳�        */
        LW_CAND_ROT(pcpu) = LW_FALSE;                                   /*  ������ȼ����Ʊ�־          */
    
    } else {                                                            /*  û���ں�ѡ����              */
        if (_PcbIsEmpty(ppcb)) {
            __DEL_RDY_MAP(ptcb);                                        /*  ��λͼ�����λ����          */
        }
    }
}
/*********************************************************************************************************
** ��������: _CandTableUpdate
** ��������: ���Խ�������ȼ���������װ���ѡ��. 
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����µĺ�ѡ����Ϊ������ǰ CPU ��������û�����ù����Ʊ�־, ���ͬʱ�������������������������
             CPU ��, ����Ҫ�������� CPU �ľ��Ʊ�־.
*********************************************************************************************************/
VOID _CandTableUpdate (PLW_CLASS_CPU   pcpu)
{
             UINT8              ucPriority;
    REGISTER PLW_CLASS_TCB      ptcbCand;
             PLW_CLASS_PCBBMAP  ppcbbmap;
             BOOL               bNeedRotate = LW_FALSE;
             
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ���                    */
             PLW_CLASS_TCB      ptcbNew;
#endif

    if (!LW_CPU_IS_ACTIVE(pcpu)) {                                      /*  CPU ����Ϊ����״̬          */
        return;
    }
    
    ptcbCand = LW_CAND_TCB(pcpu);
    if (ptcbCand == LW_NULL) {                                          /*  ��ǰû�к�ѡ�߳�            */
        _CandTableFill(pcpu);
        goto    __update_done;
    }
    
    ppcbbmap = _SchedSeekPriority(pcpu, &ucPriority);                   /*  ��ǰ��������������ȼ�      */
    if (ppcbbmap == LW_NULL) {
        LW_CAND_ROT(pcpu) = LW_FALSE;                                   /*  ������ȼ����Ʊ�־          */
        return;
    }
    
    if (ptcbCand->TCB_usSchedCounter == 0) {                            /*  �Ѿ�û��ʱ��Ƭ��            */
        if (LW_PRIO_IS_HIGH_OR_EQU(ucPriority, 
                                   ptcbCand->TCB_ucPriority)) {         /*  �Ƿ���Ҫ��ת                */
            bNeedRotate = LW_TRUE;
        }
    } else {
        if (LW_PRIO_IS_HIGH(ucPriority, 
                            ptcbCand->TCB_ucPriority)) {
            bNeedRotate = LW_TRUE;
        }
    }
    
    if (bNeedRotate) {                                                  /*  ���ڸ���Ҫ���е��߳�        */
        _CandTableEmpty(pcpu);                                          /*  ��պ�ѡ��                  */
        _CandTableResel(pcpu, ppcbbmap, ucPriority);                    /*  ����ѡ������ִ��            */
        
#if LW_CFG_SMP_EN > 0                                                   /*  SMP ���                    */
        ptcbNew = LW_CAND_TCB(pcpu);
        if (ptcbNew->TCB_bCPULock && !ptcbCand->TCB_bCPULock) {         /*  �Ƿ���Ҫ���Ա������ CPU    */
            INT             i;
            ULONG           ulCPUId = pcpu->CPU_ulCPUId;
            PLW_CLASS_CPU   pcpuOther;
            PLW_CLASS_TCB   ptcbOther;
        
            LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUId) {                 /*  CPU �����Ǽ���״̬          */
                pcpuOther = LW_CPU_GET(i);
                ptcbOther = LW_CAND_TCB(pcpuOther);
                
                if (LW_CAND_ROT(pcpuOther) == LW_FALSE) {
                    if (ptcbOther->TCB_usSchedCounter == 0) {           /*  �Ѿ�û��ʱ��Ƭ��            */
                        if (LW_PRIO_IS_HIGH_OR_EQU(ptcbCand->TCB_ucPriority, 
                                                   ptcbOther->TCB_ucPriority)) {
                            LW_CAND_ROT(pcpuOther) = LW_TRUE;
                        }
                    } else {
                        if (LW_PRIO_IS_HIGH(ptcbCand->TCB_ucPriority, 
                                            ptcbOther->TCB_ucPriority)) {
                            LW_CAND_ROT(pcpuOther) = LW_TRUE;
                        }
                    }
                }
            }
        }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
    
__update_done:
    LW_CAND_ROT(pcpu) = LW_FALSE;                                       /*  ������ȼ����Ʊ�־          */
}
/*********************************************************************************************************
** ��������: _CandTableRemove
** ��������: ��һ�� CPU ��Ӧ�ĺ�ѡ����� 
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

VOID _CandTableRemove (PLW_CLASS_CPU   pcpu)
{
    if (LW_CPU_IS_ACTIVE(pcpu)) {                                       /*  CPU ����Ϊ�Ǽ���״̬        */
        return;
    }
    
    if (LW_CAND_TCB(pcpu)) {
        _CandTableEmpty(pcpu);
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
