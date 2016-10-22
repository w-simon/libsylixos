/* @(#)clnt_generic.c	2.2 88/08/01 4.0 RPCSRC */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
/*
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */
#define __SYLIXOS_KERNEL

#include "rpc/rpc.h"
#include "string.h"
#include "stdio.h"

#if LW_CFG_NET_RPC_EN > 0
/*
 * Generic client creation: takes (hostname, program-number, protocol) and
 * returns client handle. Default options are set, which the user can
 * change using the rpc equivalent of ioctl()'s.
 */
CLIENT *clnt_create (const char *hostname, const unsigned long prog,
				 const unsigned long vers, const char *proto)
{
	int sock;
	struct hostent *h;
	struct sockaddr_in sin;
	struct timeval tv;
	CLIENT *client;

	h = gethostbyname(hostname);
	if (h == NULL) {
	    fprintf(stderr, "rpc : unknown host\n");
		return (NULL);
	}
	if (h->h_addrtype != AF_INET) {
	    fprintf(stderr, "rpc : unknow inet\n");
		return (NULL);
	}
	memset((char*)&sin,0,sizeof(sin));
	sin.sin_family = h->h_addrtype;
	sin.sin_port = 0;
	memmove((char *) &sin.sin_addr, h->h_addr, h->h_length);

	sock = -1;
	if (strcmp(proto, "udp") == 0)
	{
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		client = clntudp_create(&sin, prog, vers, tv, &sock);
		if (client == NULL) return NULL;
		clnt_control(client, CLSET_TIMEOUT, (char*)&tv);
	} 
	else if (strcmp(proto, "tcp") == 0) 
	{
	    client = clnttcp_create(&sin, prog, vers, &sock, TCPMSGSIZE, TCPMSGSIZE); /* sylixos and TCPMSGSIZE */
		if (client == NULL) {
			return (NULL);
		}
		tv.tv_sec = 25;
		tv.tv_usec = 0;
		clnt_control(client, CLSET_TIMEOUT, &tv);
	}
	else
	{
	    fprintf(stderr, "rpc : unknow protocol\n");
		return NULL;
	}

	return (client);
}

void clnt_perror(CLIENT *rpch, const char *s)
{
    fprintf(stderr, "rpc : client error:%s\n", s);
}

#endif /* LW_CFG_NET_RPC_EN > 0 */
