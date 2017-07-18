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
** ��   ��   ��: lwip_vpn.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 05 �� 25 ��
**
** ��        ��: SSL VPN Ӧ�ýӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_VPN_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "socket.h"
#include "lwip_vpn.h"
/*********************************************************************************************************
  shell ���� "vpnopen" ���������ļ���ʽʾ������:
  (����˳���� API_INetVpnClientCreate ��ͬ, ע��: �ļ��޿���, �����޿ո�)
  (����֤ʱ, ֤���ļ�����Կ�ļ���Ϊ null, ����Կ�ļ�����������ʱ, �����ֶ�Ϊ null)

  /mnt/ata0/vpn/config/ca_crt.pem
  /mnt/ata0/vpn/config/client_crt.pem
  /mnt/ata0/vpn/config/client_key.pem
  123456
  61.186.128.33
  192.168.0.123
  255.255.255.0
  192.168.0.1
  4443
  500
  1
  e2:22:32:a5:8f:94
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: __tshellVpnOpen
** ��������: ϵͳ���� "vpnopen"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __tshellVpnOpen (INT  iArgC, PCHAR  *ppcArgV)
{
    INT          iFd;
    struct stat  statFile;
    PCHAR        pcBuffer = LW_NULL;
    INT          iError;

    INT          i;
    PCHAR        pcParam[12];                                           /*  ������                      */
    PCHAR        pcTemp;

    PCHAR        pcCACrtFile;
    PCHAR        pcPrivateCrtFile;
    PCHAR        pcKeyFile;
    PCHAR        pcKeyPassword;

    INT          iPort          = 4443;
    INT          iSSLTimeoutSec = 500;
    INT          iVerifyOpt     = LW_VPN_SSL_VERIFY_OPT;
    UCHAR        ucMac[6];
    INT          iMac[6];


    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    iFd = open(ppcArgV[1], O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not read the configration file %s!\n", lib_strerror(errno));
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (fstat(iFd, &statFile) < 0) {
        close(iFd);
        fprintf(stderr, "configration file error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if ((statFile.st_size) < 1 ||
        (statFile.st_size) > 1024) {                                    /*  �����ļ�Ӧ���� 1K ����      */
        close(iFd);
        fprintf(stderr, "configration file size error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    pcBuffer = (PCHAR)__SHEAP_ALLOC((size_t)(statFile.st_size) + 1);
    if (pcBuffer == LW_NULL) {
        close(iFd);
        fprintf(stderr, "system low memory!\n");
        return  (-ERROR_SYSTEM_LOW_MEMORY);
    }

    if (read(iFd, pcBuffer, (size_t)statFile.st_size) != (ssize_t)statFile.st_size) {
        __SHEAP_FREE(pcBuffer);
        close(iFd);
        fprintf(stderr, "system low memory!\n");
        return  (-ERROR_SYSTEM_LOW_MEMORY);
    }

    close(iFd);

    /*
     *  ��Լ��˳�����������ò���
     */
    pcTemp = pcBuffer;
    while ((*pcTemp == ' ') || (*pcTemp == '\t')) {                     /*  ���Կհ��ַ�                */
        pcTemp++;
        if ((pcTemp - pcBuffer) > (size_t)statFile.st_size) {           /*  �����ļ���С                */
            fprintf(stderr, "configration file size error!\n");
            goto    __error_handle;
        }
    }

    for (i = 0; i < 11; i++) {                                          /*  ���ǰ 11 ������            */
        pcParam[i] = pcTemp;
        do {
            pcTemp++;                                                   /*  �ҵ���һ����                */
            if ((pcTemp - pcBuffer) > (size_t)statFile.st_size) {       /*  �����ļ���С                */
                fprintf(stderr, "configration file size error!\n");
                goto    __error_handle;
            }
        } while ((*pcTemp != '\r') && (*pcTemp != '\n'));
        *pcTemp = PX_EOS;                                               /*  ��ǰ��������                */
        pcTemp++;                                                       /*  ����                        */

        while ((*pcTemp == ' ')  || 
               (*pcTemp == '\t') || 
               (*pcTemp == '\n') ||
               (*pcTemp == '\r')) {
            pcTemp++;                                                   /*  ���Կհ��ַ�                */
            if ((pcTemp - pcBuffer) > (size_t)statFile.st_size) {       /*  �����ļ���С                */
                fprintf(stderr, "configration file size error!\n");
                goto    __error_handle;
            }
        }
    }

    pcParam[11] = pcTemp;
    pcBuffer[statFile.st_size] = PX_EOS;                                /*  ������һ������            */

    /*
     *  ѡ�����
     */
    if (lib_strcmp(pcParam[0], "null")) {
        pcCACrtFile = pcParam[0];
    } else {
        pcCACrtFile = LW_NULL;
    }
    if (lib_strcmp(pcParam[1], "null")) {
        pcPrivateCrtFile = pcParam[1];
    } else {
        pcPrivateCrtFile = LW_NULL;
    }
    if (lib_strcmp(pcParam[2], "null")) {
        pcKeyFile = pcParam[2];
    } else {
        pcKeyFile = LW_NULL;
    }
    if (lib_strcmp(pcParam[3], "null")) {
        pcKeyPassword = pcParam[3];
    } else {
        pcKeyPassword = LW_NULL;
    }

    sscanf(pcParam[ 8], "%d", &iPort);
    sscanf(pcParam[ 9], "%d", &iSSLTimeoutSec);
    sscanf(pcParam[10], "%d", &iVerifyOpt);
    sscanf(pcParam[11], "%x:%x:%x:%x:%x:%x", &iMac[0],
                                             &iMac[1],
                                             &iMac[2],
                                             &iMac[3],
                                             &iMac[4],
                                             &iMac[5]);
    for (i = 0; i < 6; i++) {
        ucMac[i] = (UCHAR)iMac[i];
    }

    iError = API_INetVpnClientCreate(pcCACrtFile,
                                     pcPrivateCrtFile,
                                     pcKeyFile,
                                     pcKeyPassword,
                                     pcParam[4],
                                     pcParam[5],
                                     pcParam[6],
                                     pcParam[7],
                                     htons((UINT16)iPort),
                                     iSSLTimeoutSec,
                                     iVerifyOpt,
                                     ucMac);                            /*  ���� vpn �ͻ�������         */
    if (iError < ERROR_NONE) {
        fprintf(stderr, "can not create the vpn connection!\n");
        goto    __error_handle;
    }
    
    __SHEAP_FREE(pcBuffer);

    return  (ERROR_NONE);

__error_handle:
    if (pcBuffer) {
        __SHEAP_FREE(pcBuffer);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __tshellVpnClose
** ��������: ϵͳ���� "vpnclose"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __tshellVpnClose (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    return  (API_INetVpnClientDelete(ppcArgV[1]));                      /*  �Ƴ� VPN ����ӿ�           */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_VPN_EN > 0       */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
