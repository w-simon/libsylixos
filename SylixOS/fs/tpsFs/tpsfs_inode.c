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
** ��   ��   ��: tpsfs_inode.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2015 �� 9 �� 21 ��
**
** ��        ��: inode ����

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
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs_dev_buf.h"
/*********************************************************************************************************
** ��������: __tpsFsInodeSerial
** ��������: ���к�inode�ṹ
** �䡡��  : pinode           inode�ṹָ��
**           pucBuff          ���кŻ�����
**           uiSize           ��������С
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tpsFsInodeSerial (PTPS_INODE pinode, PUCHAR pucBuff, UINT uiSize)
{
    PUCHAR  pucPos = pucBuff;

    TPS_CPU_TO_LE32(pucPos, pinode->IND_uiMagic);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_uiVersion);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_ui64Generation);
    TPS_CPU_TO_IBLK(pucPos, pinode->IND_inum);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_szData);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_szAttr);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_ui64CTime);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_ui64MTime);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_ui64ATime);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_iMode);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_iType);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_uiRefCnt);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_uiDataStart);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_iUid);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_iGid);

    tpsSerialBtrNode(&pinode->IND_data.IND_btrNode,
                     pucBuff + pinode->IND_uiDataStart,
                     uiSize - pinode->IND_uiDataStart,
                     0, pinode->IND_data.IND_btrNode.ND_uiEntrys,
                     LW_NULL, LW_NULL);
}
/*********************************************************************************************************
** ��������: __tpsFsInodeUnSerial
** ��������: �����к�inode�ṹ
** �䡡��  : pinode           inode�ṹָ��
**           pucBuff          ���кŻ�����
**           uiSize           ��������С
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __tpsFsInodeUnserial (PTPS_INODE pinode, PUCHAR pucBuff, UINT uiSize)
{
    PUCHAR  pucPos = pucBuff;

    TPS_LE32_TO_CPU(pucPos, pinode->IND_uiMagic);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_uiVersion);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_ui64Generation);
    TPS_IBLK_TO_CPU(pucPos, pinode->IND_inum);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_szData);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_szAttr);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_ui64CTime);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_ui64MTime);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_ui64ATime);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_iMode);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_iType);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_uiRefCnt);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_uiDataStart);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_iUid);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_iGid);

    tpsUnserialBtrNode(&pinode->IND_data.IND_btrNode,
                       pucBuff + pinode->IND_uiDataStart,
                       uiSize - pinode->IND_uiDataStart);
}
/*********************************************************************************************************
** ��������: tpsFsCreateInode
** ��������: ��������ʼ��inode
** �䡡��  : ptrans           ����
**           psb              ������ָ��
**           inum             �ļ���
**           iMode            �ļ�ģʽ
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsCreateInode (PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb, TPS_INUM inum, INT iMode)
{
    PTPS_INODE pinode       = LW_NULL;
    PUCHAR     pucBuff      = LW_NULL;
    UINT       uiMaxNodeCnt = 0;
	UINT       uiInodeSize  = 0;

    if (psb == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

	uiInodeSize = (psb->SB_uiBlkSize - TPS_INODE_DATASTART 
                   - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2);
	uiInodeSize *=  sizeof(TPS_BTR_KV);
	uiInodeSize += (TPS_INODE_DATASTART + sizeof(TPS_BTR_NODE));
    pinode = (PTPS_INODE)TPS_ALLOC(uiInodeSize);
    if (LW_NULL == pinode) {
        return  (TPS_ERR_ALLOC);
    }

    pinode->IND_psb             = psb;
    pinode->IND_inum            = inum;
    pinode->IND_iMode           = iMode;
    pinode->IND_iType           = TPS_INODE_TYPE_REG;
    pinode->IND_uiRefCnt        = 1;                                    /* �������ȻҪ������           */
    pinode->IND_ui64Generation  = psb->SB_ui64Generation;
    pinode->IND_uiDataStart     = TPS_INODE_DATASTART;
    pinode->IND_uiMagic         = TPS_MAGIC_INODE;
    pinode->IND_uiVersion       = psb->SB_uiVersion;
    pinode->IND_pnext           = LW_NULL;
    pinode->IND_szData          = 0;
    pinode->IND_szAttr          = 0;
    pinode->IND_ui64CTime       = TPS_UTC_TIME();
    pinode->IND_ui64MTime       = pinode->IND_ui64CTime;
    pinode->IND_ui64ATime       = pinode->IND_ui64CTime;
    pinode->IND_iUid            = getuid();
    pinode->IND_iGid            = getgid();
    lib_bzero(&pinode->IND_data, psb->SB_uiBlkSize - TPS_INODE_DATASTART);
    
    uiMaxNodeCnt = (psb->SB_uiBlkSize - TPS_INODE_DATASTART - sizeof(TPS_BTR_NODE)) / sizeof(TPS_BTR_KV);
    pinode->IND_data.IND_btrNode.ND_uiMaxCnt  = uiMaxNodeCnt;
    pinode->IND_data.IND_btrNode.ND_blkThis   = inum;
    pinode->IND_data.IND_btrNode.ND_iType     = TPS_BTR_NODE_LEAF;
    pinode->IND_data.IND_btrNode.ND_uiMagic   = TPS_MAGIC_BTRNODE;

    pucBuff = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);
    if (LW_NULL == pucBuff) {
        TPS_FREE(pinode);
        return  (TPS_ERR_ALLOC);
    }

    __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);             /* ���л�                       */

    if (tpsFsTransWrite(ptrans, psb,
                        pinode->IND_inum, 0,
                        pucBuff, psb->SB_uiBlkSize) != TPS_ERR_NONE) {  /* д����                       */
        TPS_FREE(pucBuff);
        TPS_FREE(pinode);
        return  (TPS_ERR_TRANS_WRITE);
    }

    TPS_FREE(pucBuff);
    TPS_FREE(pinode);
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsOpenInode
** ��������: ��inode
** �䡡��  : psb              ������ָ��
**           inum             �ļ���
** �䡡��  : inodeָ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PTPS_INODE  tpsFsOpenInode (PTPS_SUPER_BLOCK psb, TPS_INUM inum)
{
    PTPS_INODE  pinode      = LW_NULL;
    PUCHAR      pucBuff     = LW_NULL;
	UINT		uiInodeSize = 0;

    if (psb == LW_NULL) {
        return  (LW_NULL);
    }

    pinode = psb->SB_pinodeOpenList;
    while (pinode) {
        if (pinode->IND_inum == inum) {                                 /* ���ҵ�ǰ��inode�б�        */
            pinode->IND_uiOpenCnt++;
            return  (pinode);
        }
        pinode = pinode->IND_pnext;
    }

    pucBuff = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);
    if (LW_NULL == pucBuff) {
        return  (LW_NULL);
    }

    if (tpsFsDevBufRead(psb, inum, 0, pucBuff,
                        psb->SB_uiBlkSize) != TPS_ERR_NONE) {           /* ��ȡinode                    */
        TPS_FREE(pucBuff);
        return  (LW_NULL);
    }

	uiInodeSize = (psb->SB_uiBlkSize - TPS_INODE_DATASTART
				   - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2);
	uiInodeSize *=  sizeof(TPS_BTR_KV);
	uiInodeSize += (TPS_INODE_DATASTART + sizeof(TPS_BTR_NODE));
    pinode = (PTPS_INODE)TPS_ALLOC(uiInodeSize);
    if (LW_NULL == pinode) {
        TPS_FREE(pucBuff);
        return  (LW_NULL);
    }

    __tpsFsInodeUnserial(pinode, pucBuff, psb->SB_uiBlkSize);

   pinode->IND_pucBuff = pucBuff;

    if (pinode->IND_ui64Generation != psb->SB_ui64Generation ||
        pinode->IND_uiMagic != TPS_MAGIC_INODE) {
        TPS_FREE(pucBuff);
        TPS_FREE(pinode);
        return  (LW_NULL);
    }

    pinode->IND_psb         = psb;
    pinode->IND_uiOpenCnt   = 1;
    pinode->IND_bDirty      = LW_FALSE;
    pinode->IND_bDeleted    = LW_FALSE;
    pinode->IND_pnext       = psb->SB_pinodeOpenList;
    psb->SB_pinodeOpenList  = pinode;
    pinode->IND_psb->SB_uiInodeOpenCnt++;
    lib_memset(pinode->IND_pBNPool, 0, sizeof(pinode->IND_pBNPool));
    lib_memset(pinode->IND_pBNStatus, TPS_BN_POOL_NULL, sizeof(pinode->IND_pBNStatus));

    return  (pinode);
}
/*********************************************************************************************************
** ��������: tpsFsFlushHead
** ��������: ���inodeΪ�࣬��д��ͷ����һ������flush�ļ���С
** �䡡��  : ptrans           ����
**           pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsFlushInodeHead (PTPS_TRANS ptrans, PTPS_INODE pinode)
{
    PUCHAR              pucBuff = LW_NULL;
    PTPS_SUPER_BLOCK    psb;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    psb = pinode->IND_psb;

    if (pinode->IND_bDirty) {                                           /* inode����ͷ��д��          */
        pucBuff = pinode->IND_pucBuff;

        __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);         /* ���л�                       */

        if (tpsFsTransWrite(ptrans, psb,
                            pinode->IND_inum,
                            0,
                            pucBuff,
                            psb->SB_uiSectorSize) != TPS_ERR_NONE) {
            return  (TPS_ERR_TRANS_WRITE);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsCloseInode
** ��������: �ر�inode
** �䡡��  : pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsCloseInode (PTPS_INODE pinode)
{
    PTPS_INODE  *ppinodeIter = LW_NULL;
    INT          i;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (pinode->IND_uiOpenCnt > 0) {
        pinode->IND_uiOpenCnt--;

        if (pinode->IND_uiOpenCnt == 0) {
            if (pinode->IND_bDirty) {                                   /* inode����ͷ��д��          */
            }

            ppinodeIter = &pinode->IND_psb->SB_pinodeOpenList;
            while (*ppinodeIter) {
                if ((*ppinodeIter) == pinode) {
                    (*ppinodeIter) = (*ppinodeIter)->IND_pnext;
                    pinode->IND_psb->SB_uiInodeOpenCnt--;
                    break;
                }
                ppinodeIter = &((*ppinodeIter)->IND_pnext);
            }

            for (i = 0; i < TPS_BN_POOL_SIZE; i++) {                    /* �ͷŽڵ㻺���               */
                if (pinode->IND_pBNPool[i] != LW_NULL) {
                    TPS_FREE(pinode->IND_pBNPool[i]);
                }
            }
            TPS_FREE(pinode->IND_pucBuff);
            TPS_FREE(pinode);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsInodeAddRef
** ��������: inode���ü�����1
** �䡡��  : ptrans           ����
**           pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsInodeAddRef (PTPS_TRANS ptrans, PTPS_INODE pinode)
{
    PTPS_SUPER_BLOCK    psb     = LW_NULL;
    PUCHAR              pucBuff = LW_NULL;

    if ((ptrans == LW_NULL) || (pinode == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }
    
    if (pinode->IND_bDeleted) {
        return  (TPS_ERR_INODE_DELETED);
    }

    psb = pinode->IND_psb;

    pinode->IND_uiRefCnt++;

    pucBuff = pinode->IND_pucBuff;

    __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);             /* ���л�                       */

    if (tpsFsTransWrite(ptrans, psb,
                        pinode->IND_inum,
                        0,
                        pucBuff,
                        psb->SB_uiSectorSize) != TPS_ERR_NONE) {        /* д����                       */
        return  (TPS_ERR_TRANS_WRITE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsInodeDelRef
** ��������: inode���ü�����1
** �䡡��  : ptrans           ����
**           pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsInodeDelRef (PTPS_TRANS ptrans, PTPS_INODE pinode)
{
    PTPS_SUPER_BLOCK    psb          = LW_NULL;
    PUCHAR              pucBuff      = LW_NULL;
    PTPS_INODE         *ppinodeIter  = LW_NULL;

    if ((ptrans == LW_NULL) || (pinode == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }
    
    if (pinode->IND_bDeleted) {
        return  (TPS_ERR_INODE_DELETED);
    }

    psb = pinode->IND_psb;

    if (pinode->IND_uiRefCnt > 0) {
        pinode->IND_uiRefCnt--;
    }
    if (pinode->IND_uiRefCnt == 0) {                                    /* ����Ϊ0ʱɾ���ļ�inode       */
        if (tpsFsTruncInode(ptrans, pinode, 0) != TPS_ERR_NONE) {
            return  (TPS_ERR_INODE_TRUNC);
        }
        if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                             pinode->IND_inum, pinode->IND_inum, 1) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_INSERT);
        }

        ppinodeIter = &psb->SB_pinodeOpenList;
        while (*ppinodeIter) {                                          /* ���б��Ƴ�����ֹ�ٴα���   */
            if ((*ppinodeIter) == pinode) {
                (*ppinodeIter) = (*ppinodeIter)->IND_pnext;
                psb->SB_uiInodeOpenCnt--;
                break;
            }
            ppinodeIter = &((*ppinodeIter)->IND_pnext);
        }

        pinode->IND_bDeleted        = LW_TRUE;
        pinode->IND_uiMagic         = 0;                                /* ʹ�ÿ���Ч                   */
        pinode->IND_ui64Generation  = 0;
    }

    pucBuff = pinode->IND_pucBuff;

    __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);             /* ���л�                       */

    if (tpsFsTransWrite(ptrans, psb,
                        pinode->IND_inum,
                        0,
                        pucBuff,
                        psb->SB_uiSectorSize) != TPS_ERR_NONE) {        /* д����                       */
        return  (TPS_ERR_TRANS_WRITE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsInodeDelRef
** ��������: �ض��ļ�
** �䡡��  : ptrans           ����
**           pinode           inodeָ��
**           ui64Off          �ضϺ�ĳ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsTruncInode (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_SIZE_T size)
{
    TPS_IBLK            blkStart;
    TPS_IBLK            blkPscStart;
    TPS_IBLK            blkPscCnt;
    PTPS_SUPER_BLOCK    psb;
    PUCHAR              pucBuff = LW_NULL;

    if ((ptrans == LW_NULL) || (pinode == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (pinode->IND_bDeleted) {
        return  (TPS_ERR_INODE_DELETED);
    }

    if (size > pinode->IND_szData) {
        return  (TPS_ERR_INODE_SIZE);
    }

    psb         = pinode->IND_psb;
    blkStart    = size >> psb->SB_uiBlkShift;
    if (size & psb->SB_uiBlkMask) {
        blkStart++;
    }

    while (tpsFsBtreeTrunc(ptrans, pinode, blkStart,
                           &blkPscStart, &blkPscCnt) == TPS_ERR_NONE) {
        if (blkPscCnt <= 0) {
            break;
        }

        if ((pinode->IND_data.IND_btrNode.ND_uiEntrys <= 2) && 
            (pinode->IND_data.IND_btrNode.ND_uiLevel  == 0)) {
            pinode->IND_data.IND_btrNode.ND_uiEntrys  =  pinode->IND_data.IND_btrNode.ND_uiEntrys;
        }

        if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                             blkPscStart, blkPscStart, blkPscCnt) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_INSERT);
        }
    }

    if (blkStart < tpsFsBtreeBlkCnt(pinode)) {
        return  (TPS_ERR_BTREE_TRUNC);
    }

    pinode->IND_szData = size;
    pinode->IND_bDirty = LW_TRUE;

    pucBuff = pinode->IND_pucBuff;

    __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);             /* ���л�                       */

    if (tpsFsTransWrite(ptrans, psb,
                        pinode->IND_inum,
                        0,
                        pucBuff,
                        psb->SB_uiSectorSize) != TPS_ERR_NONE) {        /* д����                       */
        return  (TPS_ERR_TRANS_WRITE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** ��������: tpsFsInodeRead
** ��������: ��ȡ�ļ�
** �䡡��  : pinode           inodeָ��
**           off              ��ʼλ��
**           pucBuff          ������
**           szLen            ��ȡ����
** �䡡��  : �ɹ�����ʵ�ʶ�ȡ���ȣ�ʧ�ܷ���-ERRNO
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsInodeRead (PTPS_INODE pinode, TPS_OFF_T off, PUCHAR pucBuff, TPS_SIZE_T szLen)
{
    TPS_IBLK            blkLogic;
    TPS_IBLK            blkPscStart;
    TPS_IBLK            blkPscCnt;
    PTPS_SUPER_BLOCK    psb         = LW_NULL;
    TPS_SIZE_T          szCompleted = 0;
    TPS_SIZE_T          szReadLen;
    TPS_OFF_T           offBlk;

    if ((pinode == LW_NULL) || (pucBuff == LW_NULL)) {
        return  (-EINVAL);
    }

    if (off >= pinode->IND_szData) {
        return  (0);
    }

    /*
     *  TODO: �ļ���С�ɱ�����inodeͷ��ʱֱ�ӱ�����inodeͷ�У��ݲ�ʹ��
     */
#if 0
    if (pinode->IND_szData < (pinode->IND_psb->SB_uiBlkSize - TPS_INODE_DATASTART)) {
        lib_memcpy(pucItemBuf,
                   pinode->IND_data.IND_cData + off,
                   min(szLen, pinode->IND_szData - off));
        return  (min(szLen, pinode->IND_szData - off));
    }
#endif

    psb = pinode->IND_psb;

    while (szLen > 0) {
        blkLogic = off >> psb->SB_uiBlkShift;
        if (tpsFsBtreeGetBlk(pinode, blkLogic, &blkPscStart, &blkPscCnt) != TPS_ERR_NONE) {
            break;
        }

        offBlk    = off & psb->SB_uiBlkMask;
        szReadLen = psb->SB_uiBlkSize * blkPscCnt - offBlk;
        szReadLen = min(szLen, szReadLen);
        szReadLen = min(szReadLen, (pinode->IND_szData - off));

        if (tpsFsDevBufRead(psb, blkPscStart,
                            (UINT)offBlk, pucBuff + szCompleted,
                            (size_t)szReadLen) != TPS_ERR_NONE) {       /* ��ȡinode                    */
            szCompleted = (-EIO);
            break;
        }

        off         += szReadLen;
        szLen       -= szReadLen;
        szCompleted += szReadLen;

        if (off >= pinode->IND_szData) {
            break;
        }
    }

    return  (szCompleted);
}
/*********************************************************************************************************
** ��������: tpsFsInodeTransWrite
** ��������: д�ļ�
** �䡡��  : ptrans           ����
**           pinode           inodeָ��
**           off              ��ʼλ��
**           pucBuff          ������
**           szLen            ��ȡ����
**           bTransData       �ļ������Ƿ�Ҫ��¼����
** �䡡��  : �ɹ�����ʵ��д�볤�ȣ���ʧ�ܷ���-ERRNO
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsInodeWrite (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_OFF_T off,
                             PUCHAR pucBuff, TPS_SIZE_T szLen, BOOL bTransData)
{
    TPS_IBLK            blkEnd;
    TPS_IBLK            blkAlloc = 0;
    TPS_IBLK            blkPscStart;
    TPS_IBLK            blkPscCnt;

    TPS_IBLK            blkLogic;
    PTPS_SUPER_BLOCK    psb         = LW_NULL;
    TPS_SIZE_T          szCompleted = 0;
    TPS_SIZE_T          szWriteLen;
    TPS_OFF_T           offBlk;

    if ((pinode == LW_NULL) || (pucBuff == LW_NULL)) {
        return  (-EINVAL);
    }

    psb = pinode->IND_psb;
    blkEnd = (off + szLen) >> psb->SB_uiBlkShift;
    if ((off + szLen) & psb->SB_uiBlkMask) {
        blkEnd++;
    }

    blkAlloc = 0;
    if (blkEnd > tpsFsBtreeBlkCnt(pinode)) {
        blkAlloc = blkEnd - tpsFsBtreeBlkCnt(pinode);
    }
    while (blkAlloc > 0) {                                              /* ��Ҫ�·���鵽�ļ�           */
        if (tpsFsBtreeAllocBlk(ptrans, psb->SB_pinodeSpaceMng,
                               0, blkAlloc,
                               &blkPscStart, &blkPscCnt) == TPS_ERR_NONE) {
            if (tpsFsBtreeAppendBlk(ptrans, pinode, blkPscStart, blkPscCnt) != TPS_ERR_NONE) {
                return  (-EIO);
            }
            blkAlloc -= blkPscCnt;
        
        } else {
            return  (-ENOSPC);
        }
    }

    while (szLen > 0) {
        blkLogic = off >> psb->SB_uiBlkShift;
        if (tpsFsBtreeGetBlk(pinode, blkLogic,
                             &blkPscStart, &blkPscCnt) != TPS_ERR_NONE) {
            break;
        }

        offBlk = off & psb->SB_uiBlkMask;
        szWriteLen = psb->SB_uiBlkSize * blkPscCnt - offBlk;
        szWriteLen = min(szLen, szWriteLen);

        if (bTransData) {
            if (tpsFsTransWrite(ptrans, psb, blkPscStart,
                                (UINT)offBlk, pucBuff + szCompleted,
                                (size_t)szWriteLen) != TPS_ERR_NONE) {  /* ����д                       */
                szCompleted = (-EIO);
                break;
            }
        
        } else {
            if (tpsFsDevBufWrite(psb,
                                 blkPscStart,
                                 (UINT)offBlk,
                                 pucBuff + szCompleted,
                                 (size_t)szWriteLen,
                                 LW_FALSE) != TPS_ERR_NONE) {           /* ������д                     */
                szCompleted = (-EIO);
                break;
            }

            if (pinode->IND_iFlag & (O_SYNC | O_DSYNC)) {               /* ��Ҫ����ͬ��                 */
                if (tpsFsDevBufSync(psb,
                                    blkPscStart,
                                    (UINT)offBlk,
                                    (size_t)szWriteLen) != TPS_ERR_NONE) {
                    szCompleted = (-EIO);
                    break;
                }
            }
        }

        off         += szWriteLen;
        szLen       -= szWriteLen;
        szCompleted += szWriteLen;
    }

    pinode->IND_ui64MTime = TPS_UTC_TIME();
    pinode->IND_ui64ATime = pinode->IND_ui64MTime;

    if (off > pinode->IND_szData) {
        pinode->IND_szData = off;
        pinode->IND_bDirty = LW_TRUE;
        if (bTransData) {
            if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
                return  (-EIO);
            }
        }
    }

    return  (szCompleted);
}
/*********************************************************************************************************
** ��������: tpsFsInodeGetSize
** ��������: д�ļ�
** �䡡��  : pinode           inodeָ��
** �䡡��  : ��ȡinode�ļ����ݳ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsInodeGetSize (PTPS_INODE pinode)
{
    return  (pinode->IND_szData);
}
/*********************************************************************************************************
** ��������: tpsFsInodeSync
** ��������: ͬ���ļ���
** �䡡��  : pinode           inodeָ��
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
TPS_RESULT  tpsFsInodeSync (PTPS_INODE pinode)
{
    TPS_IBLK            blkPscStart;
    TPS_IBLK            blkPscCnt;
    TPS_IBLK            blkLogic        = 0;
    PTPS_SUPER_BLOCK    psb             = LW_NULL;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    psb = pinode->IND_psb;

    while (blkLogic * psb->SB_uiBlkSize < pinode->IND_szData) {
        if (tpsFsBtreeGetBlk(pinode, blkLogic,
                             &blkPscStart, &blkPscCnt) != TPS_ERR_NONE) {
            break;
        }

        if (tpsFsDevBufSync(psb,
                            blkPscStart,
                            0,
                            (size_t)(blkPscCnt << psb->SB_uiBlkShift)) != TPS_ERR_NONE) {
            break;                                                      /* ͬ����                       */
        }

        blkLogic += blkPscCnt;
    }

    if (blkLogic * psb->SB_uiBlkSize < pinode->IND_szData) {
        return  (TPS_ERR_INODE_SYNC);
    }

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
