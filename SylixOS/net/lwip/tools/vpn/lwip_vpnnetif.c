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
** ��   ��   ��: lwip_vpnnetif.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 05 �� 25 ��
**
** ��        ��: SSL VPN ��������ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_VPN_EN > 0)
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/inet.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "netif/etharp.h"
#include "polarssl/ssl.h"
#include "polarssl/havege.h"
#include "lwip_vpnlib.h"
/*********************************************************************************************************
  VPN ��������ӿ����ݰ���ʽ (��̫�����ݷ����ʽ)
  +----------+----------+----------+--------------------------+
  | Ŀ�� MAC |  Դ MAC  |   ����   |         ���ݸ���         |
  +----------+----------+----------+--------------------------+
     6 bytes    6 bytes    2 bytes          46 - 1500
*********************************************************************************************************/
/*********************************************************************************************************
  VPN ��������ӿ�����
*********************************************************************************************************/
#define VPN_NETIF_MTU           1500                                    /*  VPN ���ڵ�������ݰ�����    */
/*********************************************************************************************************
  VPN ��������ӿ���������
*********************************************************************************************************/
#define VPN_COMMAND_GETMAC      0x01                                    /*  ��ѯ MAC ��ַ               */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT     __vpnClientClose(__PVPN_CONTEXT  pvpnctx);
/*********************************************************************************************************
  VPN ȫ�ֱ���
*********************************************************************************************************/
static const u8_t       _G_ucVpnEthBoradcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
/*********************************************************************************************************
** ��������: __vpnNetifOutput
** ��������: VPN �������ڷ��ͺ���
** �䡡��  : netif         VPN ����ӿ�
**           p             ���ݰ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static err_t  __vpnNetifOutput (struct netif *netif, struct pbuf *p)
{
    __PVPN_CONTEXT  pvpnctx = (__PVPN_CONTEXT)netif->state;
    u8_t            ucBuffer[VPN_NETIF_MTU + 14];
    u8_t           *pucTemp = ucBuffer;
    INT             iTotalLen;                                          /*  �������ݰ���С, ���� 60 �ֽ�*/
    INT             iDataLen;                                           /*  ���ݰ����ݲ��ֳ���          */
    INT             i;
    struct  pbuf   *q;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);                                      /*  drop the padding word       */
#endif                                                                  /*  ETH_PAD_SIZE                */

    for(q = p; q != LW_NULL; q = q->next) {
        lib_memcpy(pucTemp, q->payload, q->len);                        /*  copy data                   */
        pucTemp += q->len;
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);                                       /*  reclaim the padding word    */
#endif                                                                  /*  ETH_PAD_SIZE                */

    iDataLen  = p->tot_len;
#if ETH_PAD_SIZE
    iDataLen -= ETH_PAD_SIZE;
#endif                                                                  /*  ETH_PAD_SIZE                */
    /*
     *  ��Ҫ�������� 60 ���ֽ�(������ CRC ����)
     */
    if (iDataLen < 60) {
        iTotalLen = 60;
        for (i = 0; i < iTotalLen - iDataLen; i++) {
            *pucTemp++ = 0x00;
        }
    } else {
        iTotalLen = iDataLen;
    }

    if (ssl_write(&pvpnctx->VPNCTX_sslctx, ucBuffer, iTotalLen) == iTotalLen) {
        if (lib_memcmp(ucBuffer, _G_ucVpnEthBoradcast, 6) == 0) {
            snmp_inc_ifoutnucastpkts(netif);
        } else {
            snmp_inc_ifoutucastpkts(netif);
        }
        snmp_add_ifoutoctets(netif, iDataLen);
        LINK_STATS_INC(link.xmit);
        return  (ERR_OK);

    } else {
        snmp_inc_ifoutdiscards(netif);
        LINK_STATS_INC(link.err);
        return  (ERR_IF);
    }
}
/*********************************************************************************************************
** ��������: __vpnNetifProc
** ��������: VPN �������ڽ����߳�
** �䡡��  : pvArg         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PVOID  __vpnNetifProc (PVOID  pvArg)
{
    __PVPN_CONTEXT   pvpnctx = (__PVPN_CONTEXT)pvArg;
    struct netif    *netif   = &pvpnctx->VPNCTX_netif;

    struct pbuf     *p;
    struct pbuf     *q;

    u8_t             ucBuffer[VPN_NETIF_MTU + 14];
    PUCHAR           pucTemp;
    INT              iRead;


    if (pvpnctx == LW_NULL) {
        return  (LW_NULL);
    }

    for (;;) {
        iRead = ssl_read(&pvpnctx->VPNCTX_sslctx, ucBuffer, sizeof(ucBuffer));
        if (iRead <= 0) {
            if (errno != ETIMEDOUT) {
                break;                                                  /*  ���ӶϿ�                    */
            }
        } else {
            if (iRead < 60) {                                           /*  �������ݰ�                  */
                /*
                 *  �������ݰ�
                 */
                switch (ucBuffer[0]) {

                case VPN_COMMAND_GETMAC:                                /*  ��ȡ MAC                    */
                    ucBuffer[0] = 0x01;
                    ucBuffer[1] = 0x06;
                    ucBuffer[2] = netif->hwaddr[0];
                    ucBuffer[3] = netif->hwaddr[1];
                    ucBuffer[4] = netif->hwaddr[2];
                    ucBuffer[5] = netif->hwaddr[3];
                    ucBuffer[6] = netif->hwaddr[4];
                    ucBuffer[7] = netif->hwaddr[5];
                    ssl_write(&pvpnctx->VPNCTX_sslctx, ucBuffer, 8);
                    break;

                default:
                    break;
                }
            } else {
                /*
                 *  ��ͨ���ݰ�
                 */
#if ETH_PAD_SIZE
                iRead += ETH_PAD_SIZE;                                  /*  allow room for Eth padding  */
#endif                                                                  /*  ETH_PAD_SIZE                */
                p = pbuf_alloc(PBUF_RAW, iRead, PBUF_POOL);
                if (p == LW_NULL) {
                    snmp_inc_ifindiscards(netif);
                    LINK_STATS_INC(link.memerr);
                    LINK_STATS_INC(link.drop);

                } else {
#if ETH_PAD_SIZE
                    pbuf_header(p, -ETH_PAD_SIZE);                      /*  drop the padding word       */
#endif
                    pucTemp = &ucBuffer[0];
                    for(q = p; q != LW_NULL; q = q->next) {
                        lib_memcpy(q->payload, pucTemp, q->len);        /*  read data into pbuf         */
                        pucTemp += q->len;
                    }
#if ETH_PAD_SIZE
                    pbuf_header(p, ETH_PAD_SIZE);                       /*  reclaim the padding word    */
#endif
                    if (lib_memcmp(ucBuffer, _G_ucVpnEthBoradcast, 6) == 0) {
                        snmp_inc_ifinnucastpkts(netif);
                    } else {
                        snmp_inc_ifinucastpkts(netif);
                    }
                    snmp_add_ifinoctets(netif, iRead - ETH_PAD_SIZE);
                    LINK_STATS_INC(link.recv);

                    if (netif->input(p, netif) != ERR_OK) {
                        pbuf_free(p);                                   /*  ʧ�����ͷ�                  */
                    }
                }
            }
        }
    }

    /*
     *  ���е�������Ҫ�ر� VPN SSL ����, ��ж������ӿ�, �ͷ���Դ.
     */
    __vpnClientClose(pvpnctx);                                          /*  �ر� ssl client             */

    netifapi_netif_remove(&pvpnctx->VPNCTX_netif);                      /*  ж����������                */

    __SHEAP_FREE(pvpnctx);                                              /*  �ͷ� VPN ������             */

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vpnNetifInit
** ��������: ��ʼ�� VPN ��������
** �䡡��  : pvpnctx       VPN ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT __vpnNetifInit (__PVPN_CONTEXT  pvpnctx, UINT8  *pucMac)
{
    struct netif *netif = &pvpnctx->VPNCTX_netif;

    if (pvpnctx == LW_NULL) {
        return  (PX_ERROR);
    }

    netif->state = (void *)pvpnctx;

    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif->hwaddr[0]  = pucMac[0];
    netif->hwaddr[1]  = pucMac[1];
    netif->hwaddr[2]  = pucMac[2];
    netif->hwaddr[3]  = pucMac[3];
    netif->hwaddr[4]  = pucMac[4];
    netif->hwaddr[5]  = pucMac[5];

    netif->mtu   = VPN_NETIF_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
                                                                        /*  ע��, ������ flag ��ʼ������*/
                                                                        /*  �� netif_add() �����ص���   */
                                                                        /*  ����Ϊ�˳���������ż����  */
    netif->name[0] = __VPN_IFNAME0;
    netif->name[1] = __VPN_IFNAME1;

    netif->hostname = "sylixos_vpn";

    netif->output     = etharp_output;
    netif->linkoutput = __vpnNetifOutput;

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_VPN_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
