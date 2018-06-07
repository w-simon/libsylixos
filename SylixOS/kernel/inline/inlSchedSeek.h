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
** ��   ��   ��: inlSchedSeek.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ��������ѯ�о����̵߳�������ȼ���������
*********************************************************************************************************/

#ifndef  __INLSCHEDSEEK_H
#define  __INLSCHEDSEEK_H

/*********************************************************************************************************
  ��������ѯ�о����̵߳�������ȼ���������
*********************************************************************************************************/

static LW_INLINE PLW_CLASS_PCBBMAP  _SchedSeekPriority (PLW_CLASS_CPU  pcpu, UINT8 *ucPriority)
{
#if LW_CFG_SMP_EN > 0
    REGISTER UINT8  ucGlobal;
    
    if (_BitmapIsEmpty(LW_CPU_RDY_BMAP(pcpu))) {
        if (LW_CPU_ONLY_AFFINITY_GET(pcpu) ||
            _BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {                     /*  ������Ϊ��                  */
            return  (LW_NULL);
        }
        
        *ucPriority = _BitmapHigh(LW_GLOBAL_RDY_BMAP());
        return  (LW_GLOBAL_RDY_PCBBMAP());                              /*  ��ȫ�־�����ѡ��            */
    
    } else {
        *ucPriority = _BitmapHigh(LW_CPU_RDY_BMAP(pcpu));               /*  ���ؾ�����������ȼ���ȡ    */
        
        if (LW_CPU_ONLY_AFFINITY_GET(pcpu) || 
            _BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {
            return  (LW_CPU_RDY_PCBBMAP(pcpu));                         /*  ѡ�񱾵ؾ�������            */
        }
        
        ucGlobal = _BitmapHigh(LW_GLOBAL_RDY_BMAP());
        if (LW_PRIO_IS_HIGH_OR_EQU(*ucPriority, ucGlobal)) {            /*  ͬ���ȼ�, ����ִ�� local    */
            return  (LW_CPU_RDY_PCBBMAP(pcpu));
        
        } else {
            *ucPriority = ucGlobal;
            return  (LW_GLOBAL_RDY_PCBBMAP());
        }
    }
    
#else
    if (_BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {                         /*  ��������������              */
        return  (LW_NULL);
    
    } else {
        *ucPriority = _BitmapHigh(LW_GLOBAL_RDY_BMAP());
        return  (LW_GLOBAL_RDY_PCBBMAP());
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}

#endif                                                                  /*  __INLSCHEDSEEK_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
