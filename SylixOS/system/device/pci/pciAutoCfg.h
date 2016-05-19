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
** ��   ��   ��: pciAutoCfg.h
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PCI �����Զ�����.
**
*********************************************************************************************************/

#ifndef __PCIAUTOCFG_H
#define __PCIAUTOCFG_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
/*********************************************************************************************************
  ����ģʽ
*********************************************************************************************************/
#undef PCI_AUTO_CFG_DEBUG
/*********************************************************************************************************
  ��������ѡ�� (_STATIC �� _RECLAIM ��ѡһ)
*********************************************************************************************************/
#define PCI_AUTO_CFG_LIST_STATIC                                        /* ��̬����                     */
#undef  PCI_AUTO_CFG_LIST_RECLAIM                                       /* ��̬�ɻ���                   */
#ifndef PCI_AUTO_CFG_LIST_MAX
#define PCI_AUTO_CFG_LIST_MAX               32                          /* �б��С                     */
#endif

#if defined(PCI_AUTO_CFG_LIST_RECLAIM)
#if defined(PCI_AUTO_CFG_LIST_STATIC)
#error "PCI_AUTO_CFG_LIST_RECLAIM and PCI_AUTO_CFG_LIST_STATIC A choice In pciAutoCfg.h"
#endif                                                                  /* PCI_AUTO_CFG_LIST_STATIC     */
#endif                                                                  /* PCI_AUTO_CFG_LIST_RECLAIM    */
/*********************************************************************************************************
  PCI_LOC ����
*********************************************************************************************************/
#define PCI_AUTO_CFG_ATTR_DEV_EXCLUDE			        (UINT8)(0x01) 	/* �豸�Ƴ�          			*/
#define PCI_AUTO_CFG_ATTR_DEV_DISPLAY			        (UINT8)(0x02)   /* ��ʾ�豸           			*/
#define PCI_AUTO_CFG_ATTR_DEV_PREFETCH			        (UINT8)(0x04)   /* �豸֧��Ԥȡ     			*/

#define PCI_AUTO_CFG_ATTR_BUS_PREFETCH			        (UINT8)(0x08) 	/* Ԥȡ              			*/
#define PCI_AUTO_CFG_ATTR_BUS_PCI				        (UINT8)(0x10)   /* PCI ��            			*/
#define PCI_AUTO_CFG_ATTR_BUS_HOST				        (UINT8)(0x20)   /* PCI ����          			*/
#define PCI_AUTO_CFG_ATTR_BUS_ISA				        (UINT8)(0x40) 	/* PCI ISA ��          			*/
#define PCI_AUTO_CFG_ATTR_BUS_4GB_IO			        (UINT8)(0x80)   /* 4G/64K IO ��ַ   			*/

/*********************************************************************************************************
  ѡ������
*********************************************************************************************************/
#define PCI_AUTO_CFG_CTL_STRUCT_COPY                    0x0000			/* ����ָ���ṹ				    */

#define PCI_AUTO_CFG_CTL_FBB_ENABLE						0x0001          /* FBB ʹ��                     */
#define PCI_AUTO_CFG_CTL_FBB_DISABLE					0x0002          /* FBB ����                     */
#define PCI_AUTO_CFG_CTL_FBB_UPDATE						0x0003          /* FBB ����                     */
#define PCI_AUTO_CFG_CTL_FBB_STATUS_GET					0x0004          /* FBB״̬��ȡ                  */

#define PCI_AUTO_CFG_CTL_MAX_LAT_FUNC_SET			    0x0008          /* ���ȴ�ʱ�亯������         */
#define PCI_AUTO_CFG_CTL_MAX_LAT_ARG_SET			    0x0009          /* ���ȴ�ʱ�亯����������     */
#define PCI_AUTO_CFG_CTL_MAX_LAT_ALL_SET				0x000a          /* ���ȴ�ʱ������             */
#define PCI_AUTO_CFG_CTL_MAX_LAT_ALL_GET				0x000b          /* ���ȴ�ʱ���ȡ             */

#define PCI_AUTO_CFG_CTL_MSG_LOG_FUNC_SET		        0x000c          /* ��־��Ϣ��������             */

#define PCI_AUTO_CFG_CTL_MAX_BUS_SET					0x0010          /* �������������               */
#define PCI_AUTO_CFG_CTL_MAX_BUS_GET					0x0011          /* �����������ȡ               */
#define PCI_AUTO_CFG_CTL_CACHE_SIZE_SET					0x0012          /* ���ø��ٻ����С             */
#define PCI_AUTO_CFG_CTL_CACHE_SIZE_GET					0x0013          /* ��ȡ���ٻ����С             */
#define PCI_AUTO_CFG_CTL_AUTO_INT_ROUTE_SET				0x0014          /* �ж�·��ʹ��                 */
#define PCI_AUTO_CFG_CTL_AUTO_INT_ROUTE_GET				0x0015          /* �ж�·��״̬                 */
#define PCI_AUTO_CFG_CTL_MEM32_LOC_SET					0x0016          /* 32 λ��Ԥȡ�ռ��ַ          */
#define PCI_AUTO_CFG_CTL_MEM32_SIZE_SET					0x0017          /* 32 λ��Ԥȡ�ռ��С          */
#define PCI_AUTO_CFG_CTL_MEMIO32_LOC_SET		    	0x0018          /* 32 λ����Ԥȡ�ռ��ַ        */
#define PCI_AUTO_CFG_CTL_MEMIO32_SIZE_SET				0x0019          /* 32 λ����Ԥȡ�ռ��С        */
#define PCI_AUTO_CFG_CTL_IO32_LOC_SET					0x001a          /* 32 λ IO �ռ��ַ            */
#define PCI_AUTO_CFG_CTL_IO32_SIZE_SET					0x001b          /* 32 λ IO �ռ��С            */
#define PCI_AUTO_CFG_CTL_IO16_LOC_SET					0x001c          /* 16 λ IO �ռ��ַ            */
#define PCI_AUTO_CFG_CTL_IO16_SIZE_SET					0x001d          /* 16 λ IO �ռ��С            */
#define PCI_AUTO_CFG_CTL_INCLUDE_FUNC_SET				0x001e          /* �����豸��������             */
#define PCI_AUTO_CFG_CTL_INT_ASSIGN_FUNC_SET	        0x001f          /* �жϷ��亯������             */
#define PCI_AUTO_CFG_CTL_BRIDGE_PRE_CONFIG_FUNC_SET		0x0020          /* ���豸׼�����ú�������       */
#define PCI_AUTO_CFG_CTL_BRIDGE_POST_CONFIG_FUNC_SET    0x0021          /* ���豸�������ú�������       */
#define PCI_AUTO_CFG_CTL_ROLLCALL_FUNC_SET				0x0022          /* ̽�⺯������                 */

#define PCI_AUTO_CFG_CTL_MEM32_SIZE_GET					0x0030          /* ��ȡ 32 λ��Ԥȡ�ռ��С     */
#define PCI_AUTO_CFG_CTL_MEMIO32_SIZE_GET				0x0031          /* ��ȡ 32 λ����Ԥȡ�ռ��С   */
#define PCI_AUTO_CFG_CTL_IO32_SIZE_GET					0x0032          /* ��ȡ 32 λ IO �ռ��С       */
#define PCI_AUTO_CFG_CTL_IO16_SIZE_GET					0x0033          /* ��ȡ 16 λ IO �ռ��С       */

#define PCI_AUTO_CFG_CTL_TEMP_SPACE_SET					0x0200          /* ��δʹ��                     */
#define PCI_AUTO_CFG_CTL_MINIMIZE_RESOURCES				0x0201          /* ��δʹ��                     */

/*********************************************************************************************************
  �ڴ���Ϣ���ƿ�
*********************************************************************************************************/
typedef struct {
	PVOID		        PACM_pvMemAddr;                                 /* �ڴ���ʼ��ַ                 */
    INT			        PACM_iMemSize;                                  /* �ڴ��С                     */
} PCI_AUTO_CFG_MEM;
/*********************************************************************************************************
  �豸��Ϣ���ƿ�
*********************************************************************************************************/
typedef struct {
    INT 		        PACL_iBus;                                      /* ���ߺ�                       */
    INT 		        PACL_iDevice;                                   /* �豸��, ��ͬ�ڲۺ�           */
    INT 		        PACL_iFunction;                                 /* ���ܺ�                       */
    INT 		        PACL_iAttribute;                                /* �豸������Ϣ                 */
    INT 		        PACL_iOffset;                                   /* �ж�·��                     */
} PCI_AUTO_CFG_LOC;
/*********************************************************************************************************
  �豸 ID ���ƿ�
*********************************************************************************************************/
typedef struct {
    PCI_AUTO_CFG_LOC    PACI_paclLoc;                                   /* �豸��Ϣ                     */
    UINT32 		        PACI_uiDevVend;                                 /* ���̼��豸 ID                */
} PCI_AUTO_CFG_ID;

typedef struct pci_auto_cfg_system {
    INT                 PACS_iIndex;                                    /* ��������                     */
    UINT32              PACS_uiBusMax;                                  /* ���������                   */

    UINT32 		        PACS_uiMem32Addr;						        /* 32 λ��Ԥȡ�ռ��ַ		    */
    UINT32 		        PACS_uiMem32Size;					            /* 32 λ��Ԥȡ�ռ��С		    */
    UINT32 		        PACS_uiMemIo32Addr;						        /* 32 λ����Ԥȡ�ռ��ַ	 	*/
    UINT32 		        PACS_uiMemIo32Size;						        /* 32 λ����Ԥȡ�ռ��С 		*/
    
    UINT32 		        PACS_uiIo32Addr;							    /* 32 λ IO �ռ��ַ	 		*/
    UINT32 		        PACS_uiIo32Size;							    /* 32 λ IO �ռ��С			*/
    UINT32 		        PACS_uiIo16Addr;							    /* 16 λ IO �ռ��ַ 			*/
    UINT32 		        PACS_uiIo16Size;							    /* 16 λ IO �ռ��С 			*/

    INT 		        PACS_iCacheSize;					            /* ���ٻ����д�С 				*/
    UINT32 		        PACS_uiLatencyMax;					            /* ���ȴ�ʱ�� 				*/
    BOOL 		        PACS_bAutoIntRouting;        		            /* �ж��Զ�·��ʹ�� 	        */
                                                                        /* �жϷ���                     */
    UINT8  		        (*PACS_pfuncIntAssign)(struct pci_auto_cfg_system *ppacsSystem,
                                               PCI_AUTO_CFG_LOC           *ppaclLoc,
                                               UINT32 	                   uiDevVend);
                                                                        /* �����豸                     */
    INT                 (*PACS_pfuncInclude)(struct pci_auto_cfg_system *ppacsSystem,
                                             PCI_AUTO_CFG_LOC           *ppaclLoc,
                                             UINT32                      uiDevVend);
                                                                        /* ������׼��                   */
    VOID                (*PACS_pfuncBridgePreInit)(struct pci_auto_cfg_system *ppacsSystem,
                                                   PCI_AUTO_CFG_LOC           *ppaclLoc,
                                                   UINT32                      uiDevVend);
                                                                        /* �����ý���                   */
    VOID                (*PACS_pfuncBridgePostInit)(struct pci_auto_cfg_system *ppacsSystem,
                                                    PCI_AUTO_CFG_LOC           *ppaclLoc,
                                                    UINT32                      uiDevVend);
                                                                        /* ̽���豸                     */
    INT                 (*PACS_pfuncRollcall)();
} PCI_AUTO_CFG_SYSTEM;

typedef INT     (*PCI_AUTO_CFG_LOG_MSG_FUNC_PTR)(CPCHAR pcFmt, INT iArg0, INT iArg1, INT iArg2,
                                                 INT iArg3, INT iArg4, INT iArg5);
typedef INT     (*PCI_AUTO_CFG_INCLUDE_FUNC_PTR)(PCI_AUTO_CFG_SYSTEM *ppacsSystem,
                                                 PCI_AUTO_CFG_LOC    *ppaclLoc,
                                                 UINT32               uiDevVend);
typedef UINT8   (*PCI_AUTO_CFG_INT_ASSIGN_FUNC_PTR)(PCI_AUTO_CFG_SYSTEM *ppacsSystem,
                                                    PCI_AUTO_CFG_LOC    *ppaclLoc,
                                                    UINT32               uiDevVend);
typedef VOID    (*PCI_AUTO_CFG_BRIDGE_PRE_INIT_FUNC_PTR)(PCI_AUTO_CFG_SYSTEM *ppacsSystem,
                                                         PCI_AUTO_CFG_LOC    *ppaclLoc,
                                                         UINT32               uiDevVend);
typedef VOID    (*PCI_AUTO_CFG_BRIDGE_POST_INIT_FUNC_PTR)(PCI_AUTO_CFG_SYSTEM *ppacsSystem,
                                                          PCI_AUTO_CFG_LOC    *ppaclLoc,
                                                          UINT32               uiDevVend);

typedef INT     (*PCI_AUTO_CFG_ROLLCALL_FUNC_PTR)();
typedef UINT8   (*PCI_AUTO_CFG_LATENCY_MAX_FUNC_PTR)(INT iBus, INT iDevice, INT iFunction, PVOID pvArg);
typedef UINT32  (*PCI_AUTO_CFG_MEM_BUS_EXTRA_FUNC_PTR)(INT iBus, INT iDevice, INT iFunction, PVOID pvArg);

/*********************************************************************************************************
  ����������
*********************************************************************************************************/
LW_API INT      API_PciAutoCfgAddrAlign(UINT32  uiBase, UINT32  uiLimit, UINT32  uiSizeReq,
                                        UINT32 *puiAlignedBase);

LW_API INT      API_PciAutoCfgBarConfig(PCI_AUTO_CFG_SYSTEM  *ppacsSystem, PCI_AUTO_CFG_LOC *ppaclFunc,
                                        UINT32  uiBarOffset, UINT32  uiSize, UINT32  uiAddrInfo);

LW_API VOID     API_PciAutoCfgFuncDisable(PCI_AUTO_CFG_LOC *ppaclFunc);
LW_API VOID     API_PciAutoCfgFuncEnable(PCI_AUTO_CFG_SYSTEM *ppacsSystem, PCI_AUTO_CFG_LOC *ppaclFunc);

LW_API INT      API_PciAutoCfgDevReset(PCI_AUTO_CFG_LOC *ppaclLoc);

LW_API INT      API_PciAutoCfgBusNumberSet(PCI_AUTO_CFG_LOC *ppaclLoc,
                                           INT iPriBus, INT iSecBus, INT iSubordinate);

LW_API INT      API_PciAutoCfgCtl(PVOID  pvOpts, INT  iCmd, PVOID  pvArg);
LW_API VOID     API_PciAutoCfgInit(PCI_AUTO_CFG_SYSTEM *ppacsSystem);

#define pciAutoCfgAddrAlign         API_PciAutoCfgAddrAlign

#define pciAutoCfgBarConfig         API_PciAutoCfgBarConfig

#define pciAutoCfgFuncDisable       API_PciAutoCfgFuncDisable
#define pciAutoCfgFuncEnable        API_PciAutoCfgFuncEnable

#define pciAutoCfgDevReset          API_PciAutoCfgDevReset

#define pciAutoCfgBusNumberSet      API_PciAutoCfgBusNumberSet

#define pciAutoCfgCtl               API_PciAutoCfgCtl
#define pciAutoCfgInit              API_PciAutoCfgInit

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIAUTOCFG_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
