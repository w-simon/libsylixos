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
** ��   ��   ��: loader.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2010 �� 04 �� 17 ��
**
** ��        ��: ģ�����

** BUG:
2010.10.24  �޸�ע��.
2011.02.20  ���� unload �ӿ�.
            ����ȫ�ַ����뱾�ط��ŵĴ���. (����)
2011.03.02  ����ָ���������ط��ű������. (����)
            ������ module ���ӳ�����.
2011.03.23  ����� c++ ��֧��.
2011.03.26  API_ModuleRunEx() ���һ������ӦΪ const ����.
2011.06.09  ���� __ldPathIsFile() �жϿ�ִ���ļ�������Ȩ��.
2011.07.20  �Զ�̬�������·��Ϊ : PATH �� LD_LIBRARY_PATH.
2011.07.29  �����������˷ܵ�һ��, ����, �����, ��̫��, ������, ����, ��������Сʱ���ҵı���, �����ҵ���
            ��� SylixOS �Խ���(����)��֧��ģ��!
2011.12.08  ���ģ���ʼ��ʧ��, ��װ��ʧ��. (����)
2012.04.12  ���ں�ģ��Ĳ���, �� root �û�û��Ȩ��. (����)
2012.12.10  moduleLoadSub װ���ӿ��ʱ����Ҫ�ṩ entry. ��Ϊ���̵�����ڲ�����. (����)
2012.12.17  moduleGetLibPath �������·��. (����)
            ���� moduleLoadSub ֻ��һ����ʱ��������ϵ�жϴ���. (����)
            API_ModuleSym() ʱ��Ҫ����. (����)
2013.01.18  װ�����������ӿ�ʱ, ���ܼ̳е�ǰ��ķ����Ƿ�ɼ�����, ��Ϊ��� dlopen ��һ����ʹ�� LOCAL 
            ��ʽ, ��������������������Ķ�̬���ӿ�, ��ʱ, ��������Ŀ�Ҳʹ�� LOCAL �� dlopen �Ŀ��޷����
            �����ض���, ��Ϊ���Ҳ����������Ŀ�����ķ���, ����, ֻҪ���Զ�װ�صĶ�̬��, ����Ϊ GLOBAL.
            ���� isql -> libunixodbc.so -> libsqlite3odbc.so(LOCAL) -> libsqlite3.so ʱ����. (����)
2013.05.21  module ���·��ű�ı���������㷨, ���������ؽӿ�.
2013.06.07  ���� API_ModuleShareRefresh ����, ���������ǰ����Ķ�̬�����Ӧ�õĹ�����Ϣ.
2014.07.26  cache text update ��������ÿ��ģ�����֮��.
2015.07.20  ���� moduleDelAndDestory �е���Ч����ָ�����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
#include "../include/loader_error.h"
#include "../elf/elf_loader.h"
/*********************************************************************************************************
  ģ�����ģ��
  
  SylixOS ʹ��һ�������ַ�ռ�ӳ���ϵ, ����������Ľ���ֻ�ܳ�Ϊһ��û�ж���Ѱַ�ռ�Ľ��̿��, ��Ҫ��Ϊ��
  ������򿪷�ʹ��, �����ж������ļ�������, �������ں˶���, ��Щ��Դ���ڽ����˳�ʱ������ϵͳ���߸��׻���.
  
  �������ʱ, ��Ҫ���� SylixOS �ṩ����ز���, ��Щ������Դ���˵�������� doc/vpmpdm Ŀ¼���ҵ�. 
  
  ÿһ������ӵ��һ���Լ�˽�е��ڴ��, ����ڴ��ͨ��ȱҳ�жϷ���, Ĭ�ϴ�СΪ�������� SO_MEM_PAGES ����, ��
  �����˳�ʱ, �����ռ佫�ᱻ�ͷ�. 
    
  ע��: dlopen() װ��ʱ, RTLD_GLOBAL �����Ч, ��˿�ķ��Ž������ڱ������ڱ���̬����. ��ס, �� linux ϵͳ
        ����, ��̬��ֻ�������ڽ�����, RTLD_GLOBAL �������˿�ķ����ڱ������ڿ��Ա���̬ʹ��.
        
        ��װ�� ko �����ں�ģ���򵼳��ķ��ű�������ں˿ɼ�.
*********************************************************************************************************/
/*********************************************************************************************************
  C++ ģ�� atexit() ģ����������
*********************************************************************************************************/
extern void __cxa_module_finalize(void *pvBase, size_t stLen, BOOL bCall);
/*********************************************************************************************************
  װ�����ж��ļ��Ƿ�ɱ�װ�� (���ж��ļ�Ȩ�޺�����, ���ж��ļ���ʽ)
*********************************************************************************************************/
extern BOOL __ldPathIsFile(CPCHAR  pcName);
/*********************************************************************************************************
** ��������: moduleCreate
** ��������: ��������ʼ��ģ��ṹ.
** �䡡��  : pcPath        �ļ�·��
**           bExportSym    �Ƿ񵼳�����
**           bIsGlobal     �����Ƿ�Ϊȫ��
**           pcInit        ģ���ʼ��������
**           pcExit        ģ���˳�������
**           pcEntry       ģ����ں�����
**           pcSection     ����ָ���� section �е������ű�
**           pvVProc       ָ��Ľ�����ģ��
** �䡡��  : �����õ�ģ��ָ�룬���ʧ�ܣ����NULL��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_LD_EXEC_MODULE  *moduleCreate (CPCHAR  pcPath,
                                         BOOL    bExportSym,
                                         BOOL    bIsGlobal,
                                         CPCHAR  pcInit,
                                         CPCHAR  pcExit,
                                         CPCHAR  pcEntry,
                                         CPCHAR  pcSection,
                                         PVOID   pvVProc)
{
    LW_LD_EXEC_MODULE *pmodule      = LW_NULL;
    size_t             stModuleSize = 0;                                /*  ģ���С                    */
    size_t             stInitOff    = 0;                                /*  Init�������Ƶ�λ��          */
    size_t             stExitOff    = 0;                                /*  Exit�������Ƶ�λ��          */
    size_t             stEntryOff   = 0;                                /*  Entry�������Ƶ�λ��         */
    size_t             stPathOff    = 0;                                /*  �ļ�·���ڽṹ���е�λ��    */
    size_t             stSectionOff = 0;

    if (lib_strlen(pcPath) >= PATH_MAX) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "module path too long!\r\n");
        return  (LW_NULL);
    }

    stModuleSize = sizeof(LW_LD_EXEC_MODULE);

    if (pcInit) {
        stInitOff = stModuleSize;                                       /*  ����ƫ�ƣ��ۼӽṹ�ڴ��С  */
        stModuleSize += lib_strlen(pcInit) + 1;
    }

    if (pcExit) {
        stExitOff = stModuleSize;
        stModuleSize += lib_strlen(pcExit) + 1;
    }

    if (pcEntry) {
        stEntryOff = stModuleSize;
        stModuleSize += lib_strlen(pcEntry) + 1;
    }

    if (pcPath) {
        stPathOff = stModuleSize;
        stModuleSize += lib_strlen(pcPath) + 1;
    }

    if (pcSection) {
        stSectionOff = stModuleSize;
        stModuleSize += lib_strlen(pcSection) + 1;
    }

    pmodule = (LW_LD_EXEC_MODULE *)LW_LD_SAFEMALLOC(stModuleSize);
    if (LW_NULL == pmodule) {
        return LW_NULL;
    }
    lib_bzero(pmodule, stModuleSize);

    pmodule->EMOD_ulMagic      = __LW_LD_EXEC_MODULE_MAGIC;
    pmodule->EMOD_ulModType    = LW_LD_MOD_TYPE_KO;
    pmodule->EMOD_ulStatus     = LW_LD_STATUS_UNLOAD;
    pmodule->EMOD_ulRefs       = 0;
    pmodule->EMOD_pvUsedArr    = LW_NULL;
    pmodule->EMOD_ulUsedCnt    = 0;
    pmodule->EMOD_bExportSym   = bExportSym;
    pmodule->EMOD_bIsGlobal    = bIsGlobal;
    pmodule->EMOD_pvFormatInfo = LW_NULL;
    pmodule->EMOD_pvproc       = (LW_LD_VPROC *)pvVProc;

    if (pcInit) {
        pmodule->EMOD_pcInit = (PCHAR)pmodule + stInitOff;
        lib_strcpy(pmodule->EMOD_pcInit, pcInit);                       /*  �����ַ���                  */
    }

    if (pcExit) {
        pmodule->EMOD_pcExit = (PCHAR)pmodule + stExitOff;
        lib_strcpy(pmodule->EMOD_pcExit, pcExit);
    }

    if (pcEntry) {
        pmodule->EMOD_pcEntry = (PCHAR)pmodule + stEntryOff;
        lib_strcpy(pmodule->EMOD_pcEntry, pcEntry);
    }

    if (pcPath) {
        pmodule->EMOD_pcModulePath = (PCHAR)pmodule + stPathOff;
        lib_strcpy(pmodule->EMOD_pcModulePath, pcPath);
    }

    if (pcSection) {
        pmodule->EMOD_pcSymSection = (PCHAR)pmodule + stSectionOff;
        lib_strcpy(pmodule->EMOD_pcSymSection, pcSection);
    }

    return  (pmodule);
}
/*********************************************************************************************************
** ��������: moduleDestory
** ��������: ����ģ��.
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT moduleDestory (LW_LD_EXEC_MODULE *pmodule)
{
    INT     i;

    if (pmodule->EMOD_pfuncDestroy) {
        pmodule->EMOD_pfuncDestroy();                                   /*  ִ��ģ�����ٺ���            */
    }

    if (pmodule->EMOD_psymbolHash) {                                    /*  ���ű�����                  */
        __moduleDeleteAllSymbol(pmodule);
        LW_LD_SAFEFREE(pmodule->EMOD_psymbolHash);
    }

    if(pmodule->EMOD_psegmentArry) {                                    /*  ��չ������                  */
        for(i = 0; i < pmodule->EMOD_ulSegCount; i++) {
            LW_LD_VMSAFEFREE(pmodule->EMOD_psegmentArry[i].ESEG_ulAddr);
        }
        LW_LD_SAFEFREE(pmodule->EMOD_psegmentArry);
    }
    
    if (pmodule->EMOD_ulModType == LW_LD_MOD_TYPE_SO) {
        LW_LD_VMSAFEFREE_AREA(pmodule->EMOD_pvBaseAddr);                /*  �����ж��                  */
    } else {
        LW_LD_VMSAFEFREE(pmodule->EMOD_pvBaseAddr);                     /*  �ں�ģ��ж��                */
    }

    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncInitArray);                      /*  ���� C++ ȫ�ֶ����������� */
    LW_LD_SAFEFREE(pmodule->EMOD_ppfuncFiniArray);

    LW_LD_SAFEFREE(pmodule->EMOD_pvUsedArr);

    LW_LD_SAFEFREE(pmodule->EMOD_pvFormatInfo);

    LW_LD_SAFEFREE(pmodule);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: moduleDelAndDestory
** ��������: ��ȫ��ģ��������߽���ģ������ɾ��ģ�飬֮������ģ��.
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT moduleDelAndDestory (LW_LD_EXEC_MODULE *pmodule)
{
    LW_LIST_RING      *pringTemp = LW_NULL;
    LW_LD_EXEC_MODULE *pmodTemp  = LW_NULL;
    LW_LD_VPROC       *pvproc;

    pvproc = pmodule->EMOD_pvproc;

    LW_VP_LOCK(pvproc);

    pringTemp = pvproc->VP_ringModules;
    if (!pringTemp) {
        LW_VP_UNLOCK(pvproc);
        return  (ERROR_NONE);
    }

    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        /*
         * �˴�ʹ����Ƕѭ������
         * ����и����㷨�����뿼��pvproc->VP_ringModules���ܻ�����ɾ�������仯��
         * �޷�������ʹ��pringTemp != pvproc->VP_ringModules���ʽ�ж��Ѿ�������ϡ�
         */
        while (pvproc->VP_ringModules && pmodTemp->EMOD_ulRefs == 0) {
            pringTemp = _list_ring_get_next(pringTemp);
            _List_Ring_Del(&pmodTemp->EMOD_ringModules, &pvproc->VP_ringModules);

            moduleDestory(pmodTemp);

            if (pvproc->VP_ringModules) {
			    pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            }
        }

        if (pvproc->VP_ringModules) {
            pringTemp = _list_ring_get_next(pringTemp);
        }
    } while (pvproc->VP_ringModules && pringTemp != pvproc->VP_ringModules);

    LW_VP_UNLOCK(pvproc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: moduleGetLibPath
** ��������: ��ÿ��ļ���·��
** �䡡��  : pcParam       �û�·������
**           pcPathBuffer  ���ҵ����ļ�·������
**           stMaxLen      ��������С
**           pcEnv         ����������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT moduleGetLibPath (CPCHAR  pcFileName, PCHAR  pcPathBuffer, size_t  stMaxLen, CPCHAR  pcEnv)
{
    CHAR    cBuffer[MAX_FILENAME_LENGTH];

    PCHAR   pcStart;
    PCHAR   pcDiv;

    if (stMaxLen < 2) {                                                 /*  ��������С����              */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__ldPathIsFile(pcFileName)) {                                   /*  �ڵ�ǰĿ¼��                */
        _PathGetFull(pcPathBuffer, stMaxLen, pcFileName);               /*  �������·��                */
        return  (ERROR_NONE);
    
    } else {                                                            /*  ����һ��·��                */
        if (lib_getenv_r(pcEnv, cBuffer, MAX_FILENAME_LENGTH)
            != ERROR_NONE) {                                            /*  ��������ֵΪ��              */
            _ErrorHandle(ENOENT);                                       /*  �޷��ҵ��ļ�                */
            return  (PX_ERROR);
        }
        
        pcPathBuffer[stMaxLen - 1] = PX_EOS;

        pcDiv = cBuffer;                                                /*  �ӵ�һ��������ʼ��          */
        do {
            pcStart = pcDiv;
            pcDiv   = lib_strchr(pcStart, ':');                         /*  ������һ�������ָ��        */
            if (pcDiv) {
                *pcDiv = PX_EOS;
                pcDiv++;
            }

            snprintf(pcPathBuffer, stMaxLen, "%s/%s", pcStart, pcFileName);
                                                                        /*  �ϲ�Ϊ������Ŀ¼            */
            if (__ldPathIsFile(pcPathBuffer)) {                         /*  ���ļ����Ա�����            */
                return  (ERROR_NONE);
            }
        } while (pcDiv);
    }

    _ErrorHandle(ENOENT);                                               /*  �޷��ҵ��ļ�                */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: moduleLoadSub
** ��������: װ�� sub �� (���� .so ��Ч)
** �䡡��  : pmodule      ��ǰģ��
**           pcLibName    ����ģ������
**           bCreate      �Ƿ���û�ҵ�ʱ���������Ϊ LW_FALSE����ֻ���в��ң�����ӵ��б�
** �䡡��  : ģ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_LD_EXEC_MODULE *moduleLoadSub (LW_LD_EXEC_MODULE *pmodule, CPCHAR pcLibName, BOOL bCreate)
{
    LW_LD_EXEC_MODULE *pmoduleNeed = LW_NULL;
    CHAR               cLibPath[MAX_FILENAME_LENGTH];
    LW_LIST_RING      *pringTemp;
    CHAR              *pcFound;
    CHAR              *pcEntry;

    if (LW_NULL == pcLibName) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "invalid parameter\r\n");
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (LW_NULL);
    }

    if (LW_NULL == pmodule) {
        return  (LW_NULL);
    }
    /*
     *  ���������������Ѿ����صĿ�
     */
    LW_VP_LOCK(pmodule->EMOD_pvproc);
    pringTemp = &pmodule->EMOD_ringModules;
    do {
        pmoduleNeed = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        pcFound = lib_strstr(pmoduleNeed->EMOD_pcModulePath, pcLibName);
        if (NULL == pcFound) {
            pringTemp = _list_ring_get_next(pringTemp);
            continue;
        }

        if (pcFound + lib_strlen(pcLibName) == pmoduleNeed->EMOD_pcModulePath
                                            +  lib_strlen(pmoduleNeed->EMOD_pcModulePath)) {
            LW_VP_UNLOCK(pmodule->EMOD_pvproc);
            return  (pmoduleNeed);                                      /*  ����Ѽ��ظ�ģ�飬ֱ�ӷ���  */
        }

        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    if (!bCreate) {                                                     /*  ���û���ҵ�, ����Ҫ����    */
        return  (LW_NULL);
    }
    
    /*
     *  ��ȡ��̬���ӿ�λ��
     */
    if (ERROR_NONE != moduleGetLibPath(pcLibName, cLibPath, MAX_FILENAME_LENGTH, "LD_LIBRARY_PATH")) {
        if (ERROR_NONE != moduleGetLibPath(pcLibName, cLibPath, MAX_FILENAME_LENGTH, "PATH")) {
            fprintf(stderr, "can not find dependent library: %s\n", pcLibName);
            _ErrorHandle(ERROR_LOADER_NO_MODULE);
            return  (LW_NULL);
        }
    }

    if (pmodule->EMOD_bIsSymbolEntry) {
        pcEntry = LW_NULL;                                              /*  ���ҵ��˽������, ��������  */
    } else {
        pcEntry = pmodule->EMOD_pcEntry;                                /*  ��Ҫ�ڴ˿���Ѱ�ҽ������    */
    }

    pmoduleNeed = moduleCreate(cLibPath,
                               LW_TRUE,
                               LW_TRUE,                                 /*  �Զ�װ�صĿ�, ���ű���Ϊȫ��*/
                               pmodule->EMOD_pcInit,
                               pmodule->EMOD_pcExit,
                               pcEntry, 
                               LW_NULL,
                               pmodule->EMOD_pvproc);
                                                                        /*  ����ģ��                    */
    if (LW_NULL == pmoduleNeed) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "create module error.\r\n");
        _ErrorHandle(ERROR_LOADER_CREATE);
        return  (LW_NULL);
    }

    pringTemp = &pmodule->EMOD_ringModules;
    LW_VP_LOCK(pmodule->EMOD_pvproc);
    _List_Ring_Add_Front(&pmoduleNeed->EMOD_ringModules, &pringTemp);
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    return  (pmoduleNeed);
}
/*********************************************************************************************************
** ��������: initArrayCall
** ��������: ���ó�ʼ�������б�. (һ��Ϊ C++ ȫ�ֶ����캯���б�)
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT initArrayCall (LW_LD_EXEC_MODULE *pmodule)
{
    INT             i;
    VOIDFUNCPTR     pfuncInit    = LW_NULL;
    LW_LD_EXEC_MODULE *pmodTemp  = LW_NULL;
    LW_LIST_RING      *pringTemp = _list_ring_get_prev(&pmodule->EMOD_ringModules);

    /*
     *  �����ִ��˳��ǳ���Ҫ, ����, �����֮�����Ҫ�ȵ�����ײ�Ŀ�ĳ�ʼ��������, Ȼ��һ��һ�����ϲ��
     *  ����, ���ڲ��ĳ�ʼ�������鰴˳��ִ�оͿ�����.
     */
    LW_VP_LOCK(pmodule->EMOD_pvproc);
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        if (pmodTemp->EMOD_ulStatus == LW_LD_STATUS_LOADED) {
            pmodTemp->EMOD_ulStatus = LW_LD_STATUS_INITED;              /*  �����캯������opendl      */

            for (i = 0; i < pmodTemp->EMOD_ulInitArrCnt; i++) {         /*  ��˳����ó�ʼ������        */
                pfuncInit = pmodTemp->EMOD_ppfuncInitArray[i];
                if (pfuncInit != LW_NULL && pfuncInit != (VOIDFUNCPTR)(~0)) {
                    LW_SOFUNC_PREPARE(pfuncInit);
                    pfuncInit();
                }
            }

            if (pmodTemp->EMOD_pfuncInit) {
                LW_SOFUNC_PREPARE(pmodTemp->EMOD_pfuncInit);
                if (pmodTemp->EMOD_pfuncInit() < 0) {
                    LW_VP_UNLOCK(pmodule->EMOD_pvproc);
                    return  (PX_ERROR);
                }
            }
        }

        pringTemp = _list_ring_get_prev(pringTemp);
    } while (pringTemp != _list_ring_get_prev(&pmodule->EMOD_ringModules));
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: finiArrayCall
** ��������: ���ý��������б�. (һ��Ϊ C++ ȫ�ֶ������������б�)
** �䡡��  : pmodule       ģ��ָ��
**           bRunFini      �Ƿ���������������.
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT finiArrayCall (LW_LD_EXEC_MODULE *pmodule, BOOL  bRunFini)
{
    INT             i;
    VOIDFUNCPTR     pfuncFini    = LW_NULL;
    LW_LD_EXEC_MODULE *pmodTemp  = pmodule;
    LW_LIST_RING      *pringTemp = &pmodule->EMOD_ringModules;

    /*
     *  �����ִ��˳��ǳ���Ҫ, ����, �����֮�����Ҫ�ȵ������ϲ�Ŀ��ж�غ�����, Ȼ��һ��һ�����²��
     *  ����, ���ڲ��ĳ�ʼ�������鰴����ִ��!
     */
    LW_VP_LOCK(pmodule->EMOD_pvproc);
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        if (pmodTemp->EMOD_ulRefs == 0 && pmodTemp->EMOD_ulStatus == LW_LD_STATUS_INITED) {
            pmodTemp->EMOD_ulStatus = LW_LD_STATUS_FINIED;

            if (pmodTemp->EMOD_pfuncExit) {
                LW_SOFUNC_PREPARE(pmodTemp->EMOD_pfuncExit);
                pmodTemp->EMOD_pfuncExit();
            }
            
            if (bRunFini) {                                             /*  ������ý�������            */
                for (i = (INT)pmodTemp->EMOD_ulFiniArrCnt - 1; i >= 0; i--) {
                    pfuncFini = pmodTemp->EMOD_ppfuncFiniArray[i];
                    if (pfuncFini != LW_NULL && pfuncFini != (VOIDFUNCPTR)(~0)) {
                        LW_SOFUNC_PREPARE(pfuncFini);
                        pfuncFini();
                    }
                }
            }

            __cxa_module_finalize(pmodTemp->EMOD_pvBaseAddr,
                                  pmodTemp->EMOD_stLen,
                                  bRunFini);                            /*  �ͷŵ�ǰģ��� cxx_atexit   */
        }

        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: moduleCleanup
** ��������: ��ʾ����ģ�����ü�����Ϊ0�������������ģ������
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT moduleCleanup (LW_LD_EXEC_MODULE *pmodule)
{
    LW_LD_EXEC_MODULE *pmodTemp  = pmodule;
    LW_LIST_RING      *pringTemp = &pmodule->EMOD_ringModules;

    LW_VP_LOCK(pmodule->EMOD_pvproc);
    do {
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        pmodTemp->EMOD_ulRefs = 0;

        pringTemp = _list_ring_get_next(pringTemp);
    } while (pringTemp != &pmodule->EMOD_ringModules);
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: moduleDelRef
** ��������: �ݼ�ģ�����ü��������Ϊ0����ݼ�����ģ�����ü�������˵ݹ�
** �䡡��  : pmodule       ģ��ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT moduleDelRef (LW_LD_EXEC_MODULE *pmodule)
{
    LW_LD_EXEC_MODULE  *pmodTemp    = pmodule;
    LW_LD_EXEC_MODULE **pmodUsedArr = LW_NULL;
    LW_LIST_RING       *pringTemp;
    BOOL                bUpdated    = LW_TRUE;
    INT                 i;

    LW_VP_LOCK(pmodule->EMOD_pvproc);
    if (pmodule->EMOD_ulRefs > 0) {
        pmodule->EMOD_ulRefs--;
    }

    while (bUpdated) {
        bUpdated  = LW_FALSE;
        pringTemp = &pmodule->EMOD_ringModules;
        do {
            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

            if (pmodTemp->EMOD_ulRefs == 0 && pmodTemp->EMOD_pvUsedArr) {
                pmodUsedArr = (LW_LD_EXEC_MODULE **)pmodTemp->EMOD_pvUsedArr;
                for (i = 0; i < pmodTemp->EMOD_ulUsedCnt; i++) {
                    if (pmodUsedArr[i] && pmodUsedArr[i]->EMOD_ulRefs > 0) {
                        pmodUsedArr[i]->EMOD_ulRefs--;
                        bUpdated = LW_TRUE;
                    }
                }

                LW_LD_SAFEFREE(pmodTemp->EMOD_pvUsedArr);
            }

            pringTemp = _list_ring_get_next(pringTemp);
        } while (pringTemp != &pmodule->EMOD_ringModules);
    }
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: moduleRebootHook
** ��������: ϵͳ��������ʱ, �����������ں�ģ��� module_exit ����
** �䡡��  : iRebootType   ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  moduleRebootHook (INT  iRebootType)
{
    LW_LD_VPROC        *pvproc = &_G_vprocKernel;
    LW_LD_EXEC_MODULE  *pmodule;
    LW_LIST_RING       *pringTemp;
    BOOL                bStart;
    BOOL                bRunFini = (iRebootType == LW_REBOOT_FORCE) ? LW_FALSE : LW_TRUE;

    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
         
        pmodule = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        
        LW_VP_UNLOCK(pvproc);
        
        moduleCleanup(pmodule);
        
        finiArrayCall(pmodule, bRunFini);
        
        LW_VP_LOCK(pvproc);
    }
    LW_VP_UNLOCK(pvproc);
}
/*********************************************************************************************************
** ��������: API_ModuleStatus
** ��������: �鿴elf�ļ���Ϣ.
** �䡡��  : pcFile        �ļ�·��
**           iFd           ��Ϣ����ļ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleStatus (CPCHAR  pcFile, INT  iFd)
{
    return  __elfStatus(pcFile, iFd);
}
/*********************************************************************************************************
** ��������: API_ModuleLoad
** ��������: ��ģ�鷽ʽ����elf�ļ�. 
             (iMode == GLOBAL && pvVProc == NULL ��ģ���������ں˶���Ч)
** �䡡��  : pcFile        �ļ�·��
**           iMode         װ��ģʽ (ȫ�ֻ��Ǿֲ�)
**           pcInit        ��ʼ�������������ΪLW_NULL����ʾ����Ҫ���ó�ʼ������
**           pcExit        ģ���˳�ʱ���еĺ���, ���ΪLW_NULL����ʾ����Ҫ�˳�����
**           pvVProc       ָ��Ľ�����ģ��
** �䡡��  : ģ����
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ������װ�� pcInit �����ű�.
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_ModuleLoad (CPCHAR  pcFile,
                       INT     iMode,
                       CPCHAR  pcInit,
                       CPCHAR  pcExit,
                       PVOID   pvVProc)
{
    return API_ModuleLoadEx(pcFile, iMode, pcInit, pcExit, LW_NULL, LW_NULL, pvVProc);
}
/*********************************************************************************************************
** ��������: API_ModuleLoad
** ��������: ��ģ�鷽ʽ����elf�ļ�. ����ָ���� sect �м��ط��ű�, ��Ҫ����װ��ϵͳģ�� 
             (iMode == GLOBAL && pvVProc == NULL ��ģ���������ں˶���Ч)
** �䡡��  : pcFile        �ļ�·��
**           iMode         װ��ģʽ (ȫ�ֻ��Ǿֲ�)
**           pcInit        ��ʼ�������������ΪLW_NULL����ʾ����Ҫ���ó�ʼ������
**           pcExit        ģ���˳�ʱ���еĺ���, ���ΪLW_NULL����ʾ����Ҫ�˳�����
**           pcEntry       ģ����ں���
**           pcSection     ָ���� section
**           pvVProc       ָ��Ľ��̿��ƿ�
** �䡡��  : ģ����
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ������װ�� pcInit �����ű�.
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_ModuleLoadEx (CPCHAR  pcFile,
                         INT     iMode,
                         CPCHAR  pcInit,
                         CPCHAR  pcExit,
                         CPCHAR  pcEntry,
                         CPCHAR  pcSection,
                         PVOID   pvVProc)
{
    CHAR               cLibPath[MAX_FILENAME_LENGTH];
    LW_LD_EXEC_MODULE *pmodule   = LW_NULL;
    LW_LD_EXEC_MODULE *pmodVProc = LW_NULL;
    LW_LD_VPROC       *pvproc;
    BOOL               bIsGlobal;
    PCHAR              pcFileName;

    if (LW_NULL == pcFile) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (LW_NULL);
    }

    if (iMode & LW_OPTION_LOADER_SYM_GLOBAL) {
        bIsGlobal = LW_TRUE;
    } else {
        bIsGlobal = LW_FALSE;
    }
    
    _PathLastName(pcFile, &pcFileName);                                 /*  ȡ���ļ���                  */

    pvproc = (LW_LD_VPROC *)pvVProc;
    if (LW_NULL == pvproc) {                                            /*  �ں�ģ��                    */
        uid_t   euid = geteuid();
        if (euid != 0) {
            _ErrorHandle(EACCES);                                       /*  û��Ȩ��                    */
            return  (LW_NULL);
        }
        pvproc = &_G_vprocKernel;
    }

    if (pvproc->VP_ringModules) {
        pmodVProc = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
    }

    if (pmodVProc) {
        pmodule = moduleLoadSub(pmodVProc, pcFileName, LW_FALSE);       /*  ���ҽ�����װ��ģ������      */
    }

    if (LW_NULL == pmodule) {
        /*
        *  ��ȡ��̬���ӿ�λ��
        */
        if (ERROR_NONE != moduleGetLibPath(pcFile, cLibPath, MAX_FILENAME_LENGTH, "LD_LIBRARY_PATH")) {
            if (ERROR_NONE != moduleGetLibPath(pcFile, cLibPath, MAX_FILENAME_LENGTH, "PATH")) {
                fprintf(stderr, "can not find dependent library: %s\n", pcFile);
                _ErrorHandle(ERROR_LOADER_NO_MODULE);
                return  (LW_NULL);
            }
        }
    
        pmodule = moduleCreate(cLibPath, LW_TRUE, bIsGlobal,
                               pcInit, pcExit, pcEntry,
                               pcSection, pvproc);                      /*  ��������ģ��                */
        if (pmodule) {
            LW_VP_LOCK(pmodule->EMOD_pvproc);
            _List_Ring_Add_Last(&pmodule->EMOD_ringModules, &pvproc->VP_ringModules);
            LW_VP_UNLOCK(pmodule->EMOD_pvproc);
        }
    } else {
        if (bIsGlobal) {                                                /*  ֻҪһ��global������Ϊglobal*/
            pmodule->EMOD_bIsGlobal = bIsGlobal;
        }

        if (LW_NULL == pmodule->EMOD_pvproc) {
            pmodule->EMOD_pvproc = (LW_LD_VPROC *)pvVProc;
        }
    }
                                                                        /*  ����ģ��                    */
    if (LW_NULL == pmodule) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "create module error\r\n");
        _ErrorHandle(ERROR_LOADER_CREATE);
        return  (LW_NULL);
    }

    LW_VP_LOCK(pmodule->EMOD_pvproc);
    pmodule->EMOD_ulRefs++;
    if (__elfListLoad(pmodule, cLibPath) < 0) {                         /*  ����elf�ļ�                 */
        LW_VP_UNLOCK(pmodule->EMOD_pvproc);
        moduleDelRef(pmodule);
        moduleDelAndDestory(pmodule);
        return  (LW_NULL);
    }
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    if (pvproc->VP_ringModules == &pmodule->EMOD_ringModules) {         /*  ������׸�ģ�����ʼ������  */
        __moduleVpPatchInit(pmodule);
    }

    if (ERROR_NONE != initArrayCall(pmodule)) {                         /*  ����c++��ʼ������           */
        pmodule->EMOD_pfuncExit = LW_NULL;                              /*  init����ʧ��ʱ������exit    */
        errno = ERROR_LOADER_UNEXPECTED;
        fprintf(stderr, "function module_init returns not 0!\n");
        API_ModuleUnload(pmodule);
        return  (LW_NULL);
    }

    return  ((PVOID)pmodule);
}
/*********************************************************************************************************
** ��������: API_ModuleUnload
** ��������: ж���Ѿ����ص�elf�ļ�.
** �䡡��  : pvModule      ģ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleUnload (PVOID  pvModule)
{
    LW_LD_EXEC_MODULE *pmodule = (LW_LD_EXEC_MODULE *)pvModule;
    LW_LD_VPROC       *pvproc  = pmodule->EMOD_pvproc;

    if ((pmodule == LW_NULL) || (pmodule->EMOD_ulMagic != __LW_LD_EXEC_MODULE_MAGIC)) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }
    
    if (pvproc == &_G_vprocKernel) {
        uid_t   euid = geteuid();
        if (euid != 0) {
            _ErrorHandle(EACCES);                                       /*  û��Ȩ��                    */
            return  (PX_ERROR);
        }
    }

    moduleDelRef(pmodule);

    finiArrayCall(pmodule, LW_TRUE);                                    /*  ����c++������������         */

    LW_VP_LOCK(pmodule->EMOD_pvproc);
    __elfListUnload(pmodule);
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    moduleDelAndDestory(pmodule);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ModuleFinish
** ��������: ���˲������ڴ�ռ�����, ������̵�һ����Ϣ
** �䡡��  : pvproc     ���̿��ƿ�ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_ModuleFinish (PVOID pvVProc)
{
    LW_LD_VPROC       *pvproc = (LW_LD_VPROC *)pvVProc;
    LW_LD_EXEC_MODULE *pmodule;

    if (pvproc == LW_NULL) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }

    if (pvproc->VP_ringModules == LW_NULL) {                            /*  �Ƿ��Ѿ������˻���          */
        return  (ERROR_NONE);
    }

    pmodule = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);

    moduleCleanup(pmodule);

    finiArrayCall(pmodule, !pvproc->VP_bForceTerm);                     /*  ����c++������������         */

    __moduleVpPatchFini(pmodule);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ModuleTerminal
** ��������: ��ս������Ѿ����ص�elf�ļ�. (����Ӧ�õ��� API_ModuleFinish ���ܴ��ô˺���)
** �䡡��  : pvproc     ���̿��ƿ�ָ��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT API_ModuleTerminal (PVOID pvVProc)
{
    LW_LD_VPROC       *pvproc = (LW_LD_VPROC *)pvVProc;
    LW_LD_EXEC_MODULE *pmodule;

    if (pvproc == LW_NULL) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }

    if (pvproc->VP_ringModules == LW_NULL) {                            /*  �Ƿ��Ѿ������˻���          */
        return  (ERROR_NONE);
    }

    pmodule = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);

    LW_VP_LOCK(pmodule->EMOD_pvproc);
    __elfListUnload(pmodule);
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);

    moduleDelAndDestory(pmodule);
    
    pvproc->VP_ringModules = LW_NULL;                                   /*  ���̲��ٺ���ģ��            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ModuleShareRefresh
** ��������: ���(ˢ��)����ϵͳ���ڹ���⹲����Ϣ�Ļ���, ֮��װ�صĹ����, �����¼��㹲������.
**           ��Ҫ���ڸ����������г�����߶�̬��, ����֮ǰ�����������д˺���,
** �䡡��  : pcFileName    ������Ӧ�ó����� (NULL ��ʾȫ�����)
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleShareRefresh (CPCHAR  pcFileName)
{
    struct stat64  stat64Get;

    if (pcFileName == LW_NULL) {
        LW_LD_VMSAFE_SHARE_ABORT((dev_t)-1, (ino64_t)-1);
        
        MONITOR_EVT(MONITOR_EVENT_ID_LOADER, MONITOR_EVENT_LOADER_REFRESH, LW_NULL);
    
    } else {
        if (stat64(pcFileName, &stat64Get)) {
            return  (PX_ERROR);
        }
    
        LW_LD_VMSAFE_SHARE_ABORT(stat64Get.st_dev, stat64Get.st_ino);
        
        MONITOR_EVT(MONITOR_EVENT_ID_LOADER, MONITOR_EVENT_LOADER_REFRESH, pcFileName);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ModuleShareConfig
** ��������: ���ù����װ����,
** �䡡��  : bShare        �Ƿ����ܶι���
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_ModuleShareConfig (BOOL  bShare)
{
    return  (LW_LD_VMSAFE_SHARE_CONFIG(bShare, LW_NULL));
}
/*********************************************************************************************************
** ��������: __moduleTreeFindSym
** ��������: �ݹ����ģ���ڷ���.
** �䡡��  : pmodule       ģ��ָ��
**           pcSymName     ������
**           pulSymVal     ����ֵ
**           iFlag         ��������
**           iLayer        ���ݹ鼶��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __moduleTreeFindSym (LW_LD_EXEC_MODULE  *pmodule,
                                CPCHAR              pcSymName,
                                addr_t             *pulSymVal,
                                INT                 iFlag,
                                INT                 iLayer)
{
    INT                 i;
    LW_LD_EXEC_MODULE **pmodUsedArr = LW_NULL;

    if (ERROR_NONE == __moduleFindSym(pmodule, pcSymName, pulSymVal, iFlag)) {
        return  (ERROR_NONE);
    }

    iLayer--;
    if (iLayer == 0) {                                                  /*  �ﵽ���ݹ鼶��            */
        return  (PX_ERROR);
    }

    pmodUsedArr = (LW_LD_EXEC_MODULE **)pmodule->EMOD_pvUsedArr;
    if (!pmodUsedArr) {
        return  (PX_ERROR);
    }

    for (i = 0; i < pmodule->EMOD_ulUsedCnt; i++) {
        if (LW_NULL == pmodUsedArr[i]) {
            continue;
        }
        
        if (__moduleTreeFindSym(pmodUsedArr[i],
                                pcSymName,
                                pulSymVal,
                                iFlag,
                                iLayer) == ERROR_NONE) {                /*  �ݹ����                    */
            return  (ERROR_NONE);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_ModuleSym
** ��������: ��ѯģ���е�ָ�����ŵ�ַ
** �䡡��  : pvModule      ģ����
**           pcName        ������
** �䡡��  : ������ַ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_ModuleSym (PVOID  pvModule, CPCHAR  pcName)
{
    LW_LD_EXEC_MODULE *pmodule = (LW_LD_EXEC_MODULE *)pvModule;
    INT                iError;
    INT                iLayer = 20;                                     /*  ���ݹ� 20 ��              */
    addr_t             ulValue = (addr_t)LW_NULL;
    
    if ((pmodule == LW_NULL) || (pmodule->EMOD_ulMagic != __LW_LD_EXEC_MODULE_MAGIC)) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (LW_NULL);
    }
    
    LW_VP_LOCK(pmodule->EMOD_pvproc);                                   /*  ��Ҫ���� vproc              */
    iError = __moduleTreeFindSym(pmodule, pcName, &ulValue, LW_LD_SYM_ANY, iLayer);
    LW_VP_UNLOCK(pmodule->EMOD_pvproc);
    
    if (iError) {
        _ErrorHandle(ERROR_LOADER_NO_SYMBOL);
    }

    return  ((PVOID)ulValue);
}
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
