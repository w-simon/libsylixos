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
** ��        ��: SSL VPN ��������ӿ�.
**
** BUG:
2011.11.22  ���� polarSSL -> V1.0.0 �޸���ؽӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_VPN_EN > 0)
#include "lwip/netif.h"
#include "lwip/inet.h"
#include "socket.h"
#include "polarssl/ssl.h"
#include "polarssl/havege.h"
#include "lwip_vpnlib.h"
/*********************************************************************************************************
** ��������: __vpnClientConnect
** ��������: VPN �ͻ������ӷ�����
** �䡡��  : pvpnctx                VPN ������ (���� VPNCTX_iVerifyOpt �г�ֵ, �����ֶα��뾭�����)
**           cpcCACrtFile           CA ֤���ļ�     .pem or .crt
**           cpcPrivateCrtFile      ˽��֤���ļ�    .pem or .crt
**           cpcKeyFile             ˽����Կ�ļ�    .pem or .key
**           cpcKeyPassword         ˽����Կ�ļ���������, �����Կ�ļ�����������, ��Ϊ NULL
**           inaddr                 SSL ��������ַ
**           usPort                 SSL �������˿�  (�����ֽ���)
**           iSSLTimeoutSec         ��ʱʱ��(��λ��, �Ƽ�: 600)
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __vpnClientOpen (__PVPN_CONTEXT  pvpnctx,
                      CPCHAR          cpcCACrtFile,
                      CPCHAR          cpcPrivateCrtFile,
                      CPCHAR          cpcKeyFile,
                      CPCHAR          cpcKeyPassword,
                      struct in_addr  inaddr,
                      u16_t           usPort,
                      INT             iSSLTimeoutSec)
{
    INT                     i;
    INT                     iError = PX_ERROR;
    struct sockaddr_in      sockaddrinRemote;
    
    (VOID)iSSLTimeoutSec;                                               /*  �µ� PolarSSL ��δʹ��      */

    if (pvpnctx == LW_NULL) {
        return  (PX_ERROR);
    }

    pvpnctx->VPNCTX_iMode   = __VPN_SSL_CLIENT;                         /*  ����Ϊ client ģʽ          */
    pvpnctx->VPNCTX_iSocket = PX_ERROR;                                 /*  û�д��� socket             */

    havege_init(&pvpnctx->VPNCTX_haveagestat);                          /*  ��ʼ�������                */

    if (pvpnctx->VPNCTX_iVerifyOpt != SSL_VERIFY_NONE) {                /*  ��Ҫ��֤֤��                */
        /*
         *  ��װ CA ֤��Ϳͻ���֤��
         */
        iError = x509parse_crtfile(&pvpnctx->VPNCTX_x509certCA, cpcCACrtFile);
        if (iError != ERROR_NONE) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "CA root certificate error.\r\n");
            return  (PX_ERROR);
        }

        iError = x509parse_crtfile(&pvpnctx->VPNCTX_x509certPrivate, cpcPrivateCrtFile);
        if (iError != ERROR_NONE) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "client certificate error.\r\n");
            goto    __error_handle;
        }

        /*
         *  ��װ RSA ˽����Կ
         */
        if (cpcKeyFile) {
            iError = x509parse_keyfile(&pvpnctx->VPNCTX_rasctx, cpcKeyFile, cpcKeyPassword);
        } else {
            iError = x509parse_keyfile(&pvpnctx->VPNCTX_rasctx, cpcPrivateCrtFile, cpcKeyPassword);
        }
        if (iError != ERROR_NONE) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "key file error.\r\n");
            goto    __error_handle;
        }
    }

    /*
     *  ���� SSL ������
     */
    pvpnctx->VPNCTX_iSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (pvpnctx->VPNCTX_iSocket < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create socket.\r\n");
        goto    __error_handle;
    }

    lib_bzero(&sockaddrinRemote, sizeof(sockaddrinRemote));
    sockaddrinRemote.sin_len    = sizeof(struct sockaddr_in);
    sockaddrinRemote.sin_family = AF_INET;
    sockaddrinRemote.sin_addr   = inaddr;
    sockaddrinRemote.sin_port   = usPort;

    if(connect(pvpnctx->VPNCTX_iSocket,
               (struct sockaddr *)&sockaddrinRemote,
               sizeof(struct sockaddr_in)) < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not connect server.\r\n");
        goto    __error_handle;
    }

    havege_init(&pvpnctx->VPNCTX_haveagestat);                          /*  ��ʼ�������                */

    /*
     *  ��ʼ�� SSL/STL
     */
    if (ssl_init(&pvpnctx->VPNCTX_sslctx) != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not init ssl context.\r\n");
        goto    __error_handle;
    }

    ssl_set_endpoint(&pvpnctx->VPNCTX_sslctx, SSL_IS_CLIENT);
    ssl_set_authmode(&pvpnctx->VPNCTX_sslctx, pvpnctx->VPNCTX_iVerifyOpt);

    ssl_set_rng(&pvpnctx->VPNCTX_sslctx, havege_random, &pvpnctx->VPNCTX_haveagestat);
    ssl_set_dbg(&pvpnctx->VPNCTX_sslctx, LW_NULL, stdout);              /*  ����Ҫ DEBUG ��Ϣ           */

    ssl_set_bio(&pvpnctx->VPNCTX_sslctx,
                net_recv, &pvpnctx->VPNCTX_iSocket,
                net_send, &pvpnctx->VPNCTX_iSocket);

    ssl_set_ciphersuites(&pvpnctx->VPNCTX_sslctx, ssl_default_ciphersuites);
    ssl_set_session(&pvpnctx->VPNCTX_sslctx, &pvpnctx->VPNCTX_sslsn);

    ssl_set_ca_chain(&pvpnctx->VPNCTX_sslctx, &pvpnctx->VPNCTX_x509certCA, LW_NULL, LW_NULL);
    ssl_set_own_cert(&pvpnctx->VPNCTX_sslctx, &pvpnctx->VPNCTX_x509certPrivate, &pvpnctx->VPNCTX_rasctx);

    ssl_set_hostname(&pvpnctx->VPNCTX_sslctx, LW_NULL);                 /*  �����÷�������              */

    for (i = 0; i < __VPN_SSL_HANDSHAKE_MAX_TIME; i++) {
        iError = ssl_handshake(&pvpnctx->VPNCTX_sslctx);                /*  ����                        */
        if (iError == ERROR_NONE) {
            break;
        } else if ((iError != POLARSSL_ERR_NET_WANT_READ) &&
                   (iError != POLARSSL_ERR_NET_WANT_WRITE)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can not handshake.\r\n");
            goto    __error_handle;
        }
    }
    if (i >= __VPN_SSL_HANDSHAKE_MAX_TIME) {
        goto    __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    if (pvpnctx->VPNCTX_iSocket >= 0) {
        net_close(pvpnctx->VPNCTX_iSocket);
    }
    x509_free(&pvpnctx->VPNCTX_x509certPrivate);
    x509_free(&pvpnctx->VPNCTX_x509certCA);
    rsa_free(&pvpnctx->VPNCTX_rasctx);
    ssl_free(&pvpnctx->VPNCTX_sslctx);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __vpnClientClose
** ��������: VPN �ͻ��˹ر����Ӳ��ͷ� ssl ������ݽṹ
** �䡡��  : pvpnctx                VPN ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __vpnClientClose (__PVPN_CONTEXT  pvpnctx)
{
    if (pvpnctx == LW_NULL) {
        return  (PX_ERROR);
    }

    ssl_close_notify(&pvpnctx->VPNCTX_sslctx);                          /*  ֪ͨ ssl �����ر�           */

    if (pvpnctx->VPNCTX_iSocket >= 0) {
        net_close(pvpnctx->VPNCTX_iSocket);
    }
    x509_free(&pvpnctx->VPNCTX_x509certPrivate);
    x509_free(&pvpnctx->VPNCTX_x509certCA);
    rsa_free(&pvpnctx->VPNCTX_rasctx);
    ssl_free(&pvpnctx->VPNCTX_sslctx);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_VPN_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
