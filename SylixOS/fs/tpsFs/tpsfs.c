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
** ��   ��   ��: tpsfs.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: tpsfs API ʵ��

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
#include "tpsfs_type.h"
#include "tpsfs_error.h"
#include "tpsfs_port.h"
#include "tpsfs_super.h"
#include "tpsfs_trans.h"
#include "tpsfs_btree.h"
#include "tpsfs_dev_buf.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs.h"
/*********************************************************************************************************
** ��������: __tpsFsCheckFileName
** ��������: ����ļ�������
** �䡡��  : pcName           �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static errno_t  __tpsFsCheckFileName (CPCHAR  pcName)
{
    register CPCHAR  pcTemp;

    /*
     *  ���ܽ��� . �� .. �ļ�
     */
    pcTemp = lib_rindex(pcName, PX_DIVIDER);
    if (pcTemp) {
        pcTemp++;
        if (*pcTemp == '\0') {                                          /*  �ļ�������Ϊ 0              */
            return  (ENOENT);
        }
        if ((lib_strcmp(pcTemp, ".")  == 0) ||
            (lib_strcmp(pcTemp, "..") == 0)) {                          /*  . , .. ���                 */
            return  (ENOENT);
        }
    } else {
        if (pcName[0] == '\0') {                                        /*  �ļ�������Ϊ 0              */
            return  (ENOENT);
        }
    }

    /*
     *  ���ܰ����Ƿ��ַ�
     */
    pcTemp = (PCHAR)pcName;
    for (; *pcTemp != '\0'; pcTemp++) {
        if (lib_strchr("\\*?<>:\"|\t\r\n", *pcTemp)) {                  /*  ���Ϸ���                  */
            return  (ENOENT);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tpsFSCreate
** ��������: �������ƴ����ļ���Ŀ¼
** �䡡��  : pinodeDir        ��Ŀ¼
**           pcFileName       �ļ�����
**           iMode            �ļ�ģʽ
** �䡡��  : ���󷵻� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PTPS_ENTRY  __tpsFSCreate (PTPS_INODE pinodeDir, CPCHAR pcFileName, INT iMode, INT *piErr)
{
    PTPS_SUPER_BLOCK    psb     = pinodeDir->IND_psb;
    TPS_INUM            inum    = 0;
    TPS_IBLK            blkPscCnt;
    PTPS_TRANS          ptrans  = LW_NULL;
    PTPS_ENTRY          pentry  = LW_NULL;

    if (piErr == LW_NULL) {
        return  (LW_NULL);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* ��������                     */
    if (ptrans == LW_NULL) {
        *piErr = EINVAL;
        return  (LW_NULL);
    }

    if (tpsFsBtreeAllocBlk(ptrans, psb->SB_pinodeSpaceMng,
                           0, 1, &inum, &blkPscCnt) != TPS_ERR_NONE) {  /* ����inode                    */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = ENOSPC;
        return  (LW_NULL);
    }

    if (tpsFsCreateInode(ptrans, psb, inum, iMode) != TPS_ERR_NONE) {   /* ��ʼ��inode,���ü���Ϊ1      */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsCreateEntry(ptrans, pinodeDir,
                         pcFileName, inum) != ERROR_NONE) {             /* ����Ŀ¼�������inode      */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    pentry = tpsFsFindEntry(pinodeDir, pcFileName);
    if (pentry == LW_NULL) {
        *piErr = ENOENT;
    }

    return  (pentry);
}
/*********************************************************************************************************
** ��������: __tpsFsWalkPath
** ��������: �����ļ�����Ŀ¼������
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
**           bFindParent      �Ƿ�ֻ������Ŀ����Ŀ¼
**           ppcSymLink       ��������λ��
** �䡡��  : �ɹ�����entry�ʧ�ܷ�������������ڸ�Ŀ¼����һ��inumberΪ0��entry�����򷵻�NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PTPS_ENTRY  __tpsFsWalkPath (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, PCHAR *ppcRemain)
{
    PCHAR   pcPathDup    = LW_NULL;
    PCHAR   pcPathRemain = LW_NULL;
    PCHAR   pcFileName   = LW_NULL;

    PTPS_ENTRY pentry    = LW_NULL;
    PTPS_ENTRY pentrySub = LW_NULL;

    if (__tpsFsCheckFileName(pcPath) != ERROR_NONE) {                   /* ����ļ�·����Ч��           */
        return  (LW_NULL);
    }

    pcPathDup = (PCHAR)TPS_ALLOC(lib_strlen(pcPath) + 1);
    if (LW_NULL == pcPathDup) {
        return  (LW_NULL);
    }
    lib_strcpy(pcPathDup, pcPath);
    
    pcPathRemain = pcPathDup;

    /*
     *  �򿪸�Ŀ¼
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1);
    if (LW_NULL == pentry) {
        TPS_FREE(pcPathDup);
        return  (LW_NULL);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1);
    
    pentry->ENTRY_offset = 0;
    pentry->ENTRY_psb    = psb;
    pentry->ENTRY_uiLen  = sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1;
    pentry->ENTRY_inum   = psb->SB_inumRoot;
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)PX_STR_DIVIDER);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        TPS_FREE(pcPathDup);
        return  (LW_NULL);
    }
    pentry->ENTRY_pinodeDir = LW_NULL;

    /*
     *  Ŀ¼����
     */
    while (pcPathRemain) {
        while (*pcPathRemain == PX_DIVIDER) {
            pcPathRemain++;
        }
        pcFileName = pcPathRemain;

        if ((*pcFileName == 0) || !S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {
            break;
        }

        while ((*pcPathRemain) && (*pcPathRemain != PX_DIVIDER)) {
            pcPathRemain++;
        }
        if (*pcPathRemain != 0) {
            *pcPathRemain++ = 0;
        }

        pentrySub = tpsFsFindEntry(pentry->ENTRY_pinode, pcFileName);   /* ����Ŀ¼��                   */
        if (LW_NULL == pentrySub) {
            break;
        }
        tpsFsEntryFree(pentry);
        pentry = pentrySub;
    }

    TPS_FREE(pcPathDup);

    if (ppcRemain) {
        *ppcRemain = (PCHAR)pcPath + (pcFileName - pcPathDup);
    }

    return  (pentry);
}
/*********************************************************************************************************
** ��������: tpsFsOpen
** ��������: ���ļ�
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
**           iFlags           ��ʽ
**           iMode            �ļ�ģʽ
**           ppinode          �������inode�ṹָ��
** �䡡��  : ����ERROR���ɹ�ppinode���inode�ṹָ�룬ʧ��ppinode���LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsOpen (PTPS_SUPER_BLOCK     psb,
                    CPCHAR               pcPath,
                    INT                  iFlags,
                    INT                  iMode,
                    PCHAR               *ppcRemain,
                    PTPS_INODE          *ppinode)
{
    PTPS_ENTRY pentry       = LW_NULL;
    PTPS_ENTRY pentryNew    = LW_NULL;
    CPCHAR     pcFileName   = LW_NULL;
    PCHAR      pcRemain     = LW_NULL;
    errno_t    iErr         = ERROR_NONE;

    if (LW_NULL == ppinode) {
        return  (EINVAL);
    }
    *ppinode = LW_NULL;

    if (LW_NULL == pcPath) {
        return  (EINVAL);
    }

    if (LW_NULL == psb) {
        return  (EFORMAT);
    }

    if (iFlags & (O_CREAT | O_TRUNC)) {
        if (!(psb->SB_uiFlags & TPS_MOUNT_FLAG_WRITE)) {
            return  (EROFS);
        }
    }

    if ((lib_strcmp(pcPath, PX_STR_DIVIDER) == 0) || (pcPath[0] == '\0')) {
        *ppinode = tpsFsOpenInode(psb, psb->SB_inumRoot);

        return  (*ppinode == LW_NULL ? EIO : ERROR_NONE);
    }

    pentry = __tpsFsWalkPath(psb, pcPath, &pcRemain);
    if (LW_NULL == pentry) {
        return  (ENOENT);
    }

    if (!S_ISLNK(pentry->ENTRY_pinode->IND_iMode) && (*pcRemain == 0)) {
        if (iFlags & O_EXCL) {                                          /* �Ի��ⷽʽ��               */
            tpsFsEntryFree(pentry);
            return  (EEXIST);
        }
    }

    if (S_ISLNK(pentry->ENTRY_pinode->IND_iMode) || (*pcRemain == 0)) {
        goto  entry_ok;
    }

    if (!(iFlags & O_CREAT)) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    pcFileName = lib_rindex(pcPath, PX_DIVIDER);                        /* ��ȡ�ļ���                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPath;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    pentryNew = __tpsFSCreate(pentry->ENTRY_pinode, pcFileName, iMode, &iErr);
    tpsFsEntryFree(pentry);
    pentry = pentryNew;
    if (LW_NULL == pentry) {
        return  (iErr);
    }

entry_ok:
    *ppinode = tpsFsOpenInode(psb, pentry->ENTRY_inum);
    tpsFsEntryFree(pentry);
    if (*ppinode == LW_NULL) {
        return  (EIO);
    }

    /*
     *  �ض��ļ�,ֻ�е����Ƿ�������ʱ����Ч
     */
    if (!S_ISLNK((*ppinode)->IND_iMode)) {
        if (iFlags & O_TRUNC) {
            iErr = tpsFsTrunc(*ppinode, 0);
            if (iErr != ERROR_NONE) {
                tpsFsCloseInode(*ppinode);
                *ppinode = LW_NULL;
                return  (iErr);
            }
        }
    }

    if (ppcRemain) {
        *ppcRemain = pcRemain;
    }

    (*ppinode)->IND_iFlag = iFlags;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsRemove
** ��������: ɾ��entry
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsRemove (PTPS_SUPER_BLOCK psb, CPCHAR pcPath)
{
    PTPS_ENTRY pentry       = LW_NULL;
    PTPS_TRANS ptrans       = LW_NULL;
    PCHAR      pcRemain     = LW_NULL;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    pentry = __tpsFsWalkPath(psb, pcPath, &pcRemain);
    if (LW_NULL == pentry) {
        return  (ENOENT);
    }

    if (*pcRemain != 0) {
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* ��������                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentry);
        return  (ENOMEM);
    }

    if (tpsFsInodeDelRef(ptrans,
                         pentry->ENTRY_pinode) != TPS_ERR_NONE) {       /* inode���ü�1                 */
        tpsFsEntryFree(pentry);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsEntryRemove(ptrans, pentry) != TPS_ERR_NONE) {             /* �Ƴ�entry                    */
        tpsFsEntryFree(pentry);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentry);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsMove
** ��������: �ƶ�entry
** �䡡��  : psb              super blockָ��
**           pcPathSrc        Դ�ļ�·��
**           pcPathDst        Ŀ���ļ�·��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsMove (PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst)
{
    PTPS_ENTRY pentrySrc        = LW_NULL;
    PTPS_ENTRY pentryDst        = LW_NULL;
    PTPS_TRANS ptrans           = LW_NULL;
    CPCHAR     pcFileName       = LW_NULL;
    PCHAR      pcRemain         = LW_NULL;

    if ((LW_NULL == psb) || (LW_NULL == pcPathSrc) || (LW_NULL == pcPathDst)) {
        return  (EINVAL);
    }

    pentrySrc = __tpsFsWalkPath(psb, pcPathSrc, &pcRemain);
    if (LW_NULL == pentrySrc) {
        return  (ENOENT);
    }

    pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain);
    if (LW_NULL == pentryDst) {
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    if (*pcRemain == 0) {                                               /* �ļ��Ѵ���                   */
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (EEXIST);
    }

    pcFileName = lib_rindex(pcPathDst, PX_DIVIDER);                     /* ��ȡ�ļ���                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPathDst;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* ��������                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        return  (ENOMEM);
    }

    if (tpsFsCreateEntry(ptrans, pentryDst->ENTRY_pinode,
                         pcFileName, pentrySrc->ENTRY_inum) != TPS_ERR_NONE) {
                                                                        /* ����Ŀ¼�������inode      */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsEntryRemove(ptrans, pentrySrc) != TPS_ERR_NONE) {          /* �Ƴ�entry                    */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentrySrc);
    tpsFsEntryFree(pentryDst);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsLink
** ��������: ����Ӳ����
** �䡡��  : psb              super blockָ��
**           pcPathSrc        Դ�ļ�·��
**           pcPathDst        Ŀ���ļ�·��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsLink (PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst)
{
    PTPS_ENTRY pentrySrc        = LW_NULL;
    PTPS_ENTRY pentryDst        = LW_NULL;
    PTPS_TRANS ptrans           = LW_NULL;
    CPCHAR     pcFileName       = LW_NULL;
    PCHAR      pcRemain         = LW_NULL;

    if ((LW_NULL == psb) || (LW_NULL == pcPathSrc) || (LW_NULL == pcPathDst)) {
        return  (EINVAL);
    }

    pentrySrc = __tpsFsWalkPath(psb, pcPathSrc, &pcRemain);
    if (LW_NULL == pentrySrc) {
        return  (ENOENT);
    }

    pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain);
    if (LW_NULL == pentryDst) {
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    if (*pcRemain == 0) {                                               /* �ļ��Ѵ���                   */
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (EEXIST);
    }

    pcFileName = lib_rindex(pcPathDst, PX_DIVIDER);                     /* ��ȡ�ļ���                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPathDst;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* ��������                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        return  (ENOMEM);
    }

    if (tpsFsCreateEntry(ptrans, pentryDst->ENTRY_pinode,
                         pcFileName, pentrySrc->ENTRY_inum) != TPS_ERR_NONE) {
                                                                        /* ����Ŀ¼�������inode      */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsInodeAddRef(ptrans,
                         pentrySrc->ENTRY_pinode) != ERROR_NONE) {      /* inode���ü�1                 */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentrySrc);
    tpsFsEntryFree(pentryDst);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsRead
** ��������: ��ȡ�ļ�
** �䡡��  : pinode           inodeָ��
**           off              ��ʼλ��
**           pucItemBuf       ������
**           szLen            ��ȡ����
**           pszRet           �������ʵ�ʶ�ȡ����
** �䡡��  : ����ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsRead (PTPS_INODE   pinode,
                    PUCHAR       pucBuff,
                    TPS_OFF_T    off,
                    TPS_SIZE_T   szLen,
                    TPS_SIZE_T  *pszRet)
{
    errno_t  iErr;
    
    if (LW_NULL == pszRet) {
        return  (EINVAL);
    }

    *pszRet = 0;

    if ((LW_NULL == pinode) || (LW_NULL == pucBuff)) {
        return  (EINVAL);
    }

    *pszRet = tpsFsInodeRead(pinode, off, pucBuff, szLen);
    if (*pszRet < 0) {
        iErr = (errno_t)(-(*pszRet));
        return  (iErr);
    
    } else {
        return  (TPS_ERR_NONE);
    }
}
/*********************************************************************************************************
** ��������: tpsFsWrite
** ��������: д�ļ�
** �䡡��  : pinode           inodeָ��
**           off              ��ʼλ��
**           pucBuff          ������
**           szLen            ��ȡ����
**           pszRet           �������ʵ��д�볤��
** �䡡��  : ����ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsWrite (PTPS_INODE  pinode,
                     PUCHAR      pucBuff,
                     TPS_OFF_T   off,
                     TPS_SIZE_T  szLen,
                     TPS_SIZE_T *pszRet)
{
    PTPS_TRANS  ptrans = LW_NULL;
    errno_t     iErr;

    if (LW_NULL == pszRet) {
        return  (EINVAL);
    }

    *pszRet = 0;

    if ((LW_NULL == pinode) || (LW_NULL == pucBuff)) {
        return  (EINVAL);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* ��������                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    *pszRet = tpsFsInodeWrite(ptrans, pinode, off, pucBuff, szLen, LW_FALSE);
    if (*pszRet < 0) {
        tpsFsTransRollBackAndFree(ptrans);
        iErr = (errno_t)(-(*pszRet));
        return  (iErr);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsClose
** ��������: �ر��ļ�
** �䡡��  : pinode           �ļ�inodeָ��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsClose (PTPS_INODE pinode)
{
    PTPS_TRANS  ptrans      = LW_NULL;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* ��������                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (tpsFsCloseInode(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsFlushHead
** ��������: �ύ�ļ�ͷ�����޸�
** �䡡��  : pinode           �ļ�inodeָ��
** �䡡��  : �ɹ�����0��ʧ�ܷ��ش�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsFlushHead (PTPS_INODE pinode)
{
    PTPS_TRANS  ptrans  = LW_NULL;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* ��������                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsTrunc
** ��������: �ض��ļ�
** �䡡��  : pinode           inodeָ��
**           ui64Off          �ضϺ�ĳ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsTrunc (PTPS_INODE pinode, TPS_SIZE_T szNewSize)
{
    PTPS_TRANS  ptrans  = LW_NULL;
    errno_t     iErr    = ERROR_NONE;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* ��������                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsTruncInode(ptrans, pinode, szNewSize) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* �ύ����                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (iErr);
}
/*********************************************************************************************************
** ��������: ����Ŀ¼
** ��������: ���ļ�
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
**           iFlags           ��ʽ
**           iMode            �ļ�ģʽ
** �䡡��  : �ɹ�����inode�ṹָ�룬ʧ�ܷ���NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsMkDir (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, INT iFlags, INT iMode)
{
    PTPS_INODE  pinode;
    errno_t     iErr    = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    iMode &= ~S_IFMT;

    iErr = tpsFsOpen(psb, pcPath, iFlags | O_CREAT,
                     iMode | S_IFDIR, LW_NULL, &pinode);
    if (iErr != ERROR_NONE) {
        return  (iErr);
    }

    return  (tpsFsClose(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsOpenDir
** ��������: ��Ŀ¼
** �䡡��  : psb              super blockָ��
**           pcPath           �ļ�·��
**           ppdir            �������TPS_DIRָ��
** �䡡��  : ����ERROR���ɹ�ppdir���TPS_DIRָ�룬ʧ��ppdir���LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsOpenDir (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, PTPS_DIR *ppdir)
{
    errno_t        iErr    = ERROR_NONE;

    if (LW_NULL == ppdir) {
        return  (EINVAL);
    }

    *ppdir = LW_NULL;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    *ppdir = (PTPS_DIR)TPS_ALLOC(sizeof(TPS_DIR));
    if (LW_NULL == *ppdir) {
        return  (ENOMEM);
    }

    iErr = tpsFsOpen(psb, pcPath, O_RDWR, 0, LW_NULL, &(*ppdir)->DIR_pinode);
    if (iErr != ERROR_NONE) {
        return  (iErr);
    }

    (*ppdir)->DIR_pentry = LW_NULL;
    (*ppdir)->DIR_offPos = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsCloseDir
** ��������: �ر�Ŀ¼
** �䡡��  : pdir             TPS_DIR�ṹָ��
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsCloseDir (PTPS_DIR pdir)
{
    if (LW_NULL == pdir) {
        return  (EINVAL);
    }

    if (pdir->DIR_pentry) {
        tpsFsEntryFree(pdir->DIR_pentry);
    }

    if (pdir->DIR_pinode) {
        tpsFsCloseInode(pdir->DIR_pinode);
    }

    TPS_FREE(pdir);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsReadDir
** ��������: ��ȡĿ¼
** �䡡��  : pdir             TPS_DIR�ṹָ��
**           ppentry          �������entryָ��
** �䡡��  : ����ERROR���ɹ�ppentry���entryָ�룬ʧ��ppentry���LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsReadDir (PTPS_DIR pdir, PTPS_ENTRY *ppentry)
{
    if (LW_NULL == ppentry) {
        return  (EINVAL);
    }

    *ppentry = LW_NULL;

    if ((LW_NULL == pdir) || (LW_NULL == pdir->DIR_pinode)) {
        return  (EINVAL);
    }

    if (pdir->DIR_pentry) {
        tpsFsEntryFree(pdir->DIR_pentry);
    }

    pdir->DIR_pentry = tpsFsEntryRead(pdir->DIR_pinode, pdir->DIR_offPos);
    *ppentry = pdir->DIR_pentry;

    if (pdir->DIR_pentry) {
        pdir->DIR_offPos = pdir->DIR_pentry->ENTRY_offset + pdir->DIR_pentry->ENTRY_uiLen;
    } else {
        return  (ENOENT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsSync
** ��������: ͬ���ļ�
** �䡡��  : pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsSync (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (tpsFsInodeSync(pinode) != TPS_ERR_NONE) {
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsVolSync
** ��������: ͬ�������ļ�ϵͳ����
** �䡡��  : psb               ������ָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsVolSync (PTPS_SUPER_BLOCK psb)
{
    if (LW_NULL == psb) {
        return  (EINVAL);
    }

    if (tpsFsDevBufSync(psb,
                        psb->SB_ui64DataStartBlk,
                        0,
                        (size_t)(psb->SB_ui64DataBlkCnt << psb->SB_uiBlkShift)) != TPS_ERR_NONE) {
                                                                        /* ͬ���������ݿ�               */
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsStat
** ��������: tpsfs ����ļ� stat
** �䡡��  : psb              �ļ�ϵͳ������
**           pinode           inodeָ��
**           pstat            ��õ� stat
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#ifndef WIN32

VOID  tpsFsStat (PTPS_SUPER_BLOCK  psb, PTPS_INODE  pinode, struct stat *pstat)
{
    if (pinode) {
        if (LW_NULL == pinode->IND_psb) {
            lib_memset(pstat, 0, sizeof(struct stat));

        } else {
            pstat->st_dev     = (dev_t)pinode->IND_psb->SB_dev;
            pstat->st_ino     = (ino_t)pinode->IND_inum;
            pstat->st_mode    = pinode->IND_iMode;
            pstat->st_nlink   = pinode->IND_uiRefCnt;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = pinode->IND_szData;
            pstat->st_atime   = pinode->IND_ui64ATime;
            pstat->st_mtime   = pinode->IND_ui64MTime;
            pstat->st_ctime   = pinode->IND_ui64CTime;
            pstat->st_blksize = pinode->IND_psb->SB_uiBlkSize;
            pstat->st_blocks  = 0;
        }

    } else if (psb) {
        pstat->st_dev     = (dev_t)psb;
        pstat->st_ino     = (ino_t)psb->SB_inumRoot;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  ��֧��                      */
        pstat->st_gid     = 0;                                          /*  ��֧��                      */
        pstat->st_rdev    = 1;                                          /*  ��֧��                      */
        pstat->st_atime   = (time_t)psb->SB_ui64Generation;
        pstat->st_mtime   = (time_t)psb->SB_ui64Generation;
        pstat->st_ctime   = (time_t)psb->SB_ui64Generation;
        pstat->st_blksize = psb->SB_uiBlkSize;
        pstat->st_blocks  = (blkcnt_t)psb->SB_ui64DataBlkCnt;

        pstat->st_mode = S_IFDIR;
        if (psb->SB_uiFlags & TPS_MOUNT_FLAG_READ) {
            pstat->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
        }
        if (psb->SB_uiFlags & TPS_MOUNT_FLAG_WRITE) {
            pstat->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
        }
        
    } else {
        pstat->st_dev     = (dev_t)TPS_SUPER_MAGIC;
        pstat->st_ino     = (ino_t)TPS_SUPER_MAGIC;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  ��֧��                      */
        pstat->st_gid     = 0;                                          /*  ��֧��                      */
        pstat->st_rdev    = 1;                                          /*  ��֧��                      */
        pstat->st_atime   = (time_t)TPS_UTC_TIME();
        pstat->st_mtime   = (time_t)TPS_UTC_TIME();
        pstat->st_ctime   = (time_t)TPS_UTC_TIME();
        pstat->st_blksize = 512;
        pstat->st_blocks  = (blkcnt_t)1;
        pstat->st_mode    = S_IFDIR | DEFAULT_DIR_PERM;
    }
}

#endif                                                                  /*  WIN32                       */
/*********************************************************************************************************
** ��������: tpsFsStatfs
** ��������: tpsfs ����ļ�ϵͳ statfs
** �䡡��  : psb              �ļ�ϵͳ������
**           pstatfs            ��õ� statfs
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  tpsFsStatfs (PTPS_SUPER_BLOCK  psb, struct statfs *pstatfs)
{
    if ((LW_NULL == psb) || (LW_NULL == pstatfs)) {
        return;
    }

    pstatfs->f_bsize  = (long)psb->SB_uiBlkSize;
    pstatfs->f_blocks = (long)psb->SB_ui64DataBlkCnt;
    pstatfs->f_bfree  = (long)tpsFsBtreeGetBlkCnt(psb->SB_pinodeSpaceMng);
    pstatfs->f_bavail = pstatfs->f_bfree;
}
/*********************************************************************************************************
** ��������: tpsFsGetSize
** ��������: ��ȡ�ļ���С
** �䡡��  : pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsGetSize (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (0);
    }

    return  (pinode->IND_szData);
}
/*********************************************************************************************************
** ��������: tpsFsGetmod
** ��������: ��ȡ�ļ�ģʽ
** �䡡��  : pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  tpsFsGetmod (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (0);
    }

    return  (pinode->IND_iMode);
}
/*********************************************************************************************************
** ��������: tpsFsChmod
** ��������: �޸��ļ�ģʽ
** �䡡��  : pinode           inodeָ��
**           iMode            ģʽ
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsChmod (PTPS_INODE pinode, INT iMode)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;

    pinode->IND_iMode  &= S_IFMT;
    pinode->IND_iMode  |= iMode;
    pinode->IND_bDirty  = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsChown
** ��������: �޸��ļ�������
** �䡡��  : pinode           inodeָ��
**           uid              �û�id
**           gid              �û���id
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsChown (PTPS_INODE pinode, uid_t uid, gid_t gid)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    pinode->IND_iUid    = uid;
    pinode->IND_iGid    = gid;
    pinode->IND_bDirty  = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** ��������: tpsFsChtime
** ��������: �޸��ļ�ʱ��
** �䡡��  : pinode           inodeָ��
**           utim             ʱ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
errno_t  tpsFsChtime (PTPS_INODE pinode, struct utimbuf  *utim)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    pinode->IND_ui64ATime = utim->actime;
    pinode->IND_ui64MTime = utim->modtime;
    pinode->IND_bDirty    = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
