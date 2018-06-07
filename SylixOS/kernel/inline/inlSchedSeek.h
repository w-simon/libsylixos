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
** 文   件   名: inlSchedSeek.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统调度器查询有就绪线程的最高优先级内联函数
*********************************************************************************************************/

#ifndef  __INLSCHEDSEEK_H
#define  __INLSCHEDSEEK_H

/*********************************************************************************************************
  调度器查询有就绪线程的最高优先级内联函数
*********************************************************************************************************/

static LW_INLINE PLW_CLASS_PCBBMAP  _SchedSeekPriority (PLW_CLASS_CPU  pcpu, UINT8 *ucPriority)
{
#if LW_CFG_SMP_EN > 0
    REGISTER UINT8  ucGlobal;
    
    if (_BitmapIsEmpty(LW_CPU_RDY_BMAP(pcpu))) {
        if (LW_CPU_ONLY_AFFINITY_GET(pcpu) ||
            _BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {                     /*  就绪表为空                  */
            return  (LW_NULL);
        }
        
        *ucPriority = _BitmapHigh(LW_GLOBAL_RDY_BMAP());
        return  (LW_GLOBAL_RDY_PCBBMAP());                              /*  从全局就绪表选择            */
    
    } else {
        *ucPriority = _BitmapHigh(LW_CPU_RDY_BMAP(pcpu));               /*  本地就绪表最高优先级获取    */
        
        if (LW_CPU_ONLY_AFFINITY_GET(pcpu) || 
            _BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {
            return  (LW_CPU_RDY_PCBBMAP(pcpu));                         /*  选择本地就绪任务            */
        }
        
        ucGlobal = _BitmapHigh(LW_GLOBAL_RDY_BMAP());
        if (LW_PRIO_IS_HIGH_OR_EQU(*ucPriority, ucGlobal)) {            /*  同优先级, 优先执行 local    */
            return  (LW_CPU_RDY_PCBBMAP(pcpu));
        
        } else {
            *ucPriority = ucGlobal;
            return  (LW_GLOBAL_RDY_PCBBMAP());
        }
    }
    
#else
    if (_BitmapIsEmpty(LW_GLOBAL_RDY_BMAP())) {                         /*  就绪表中无任务              */
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
