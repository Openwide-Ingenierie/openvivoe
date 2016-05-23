/*
 * Licence: GPL
 * Created: Mon, 23 May 2016 10:54:38 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#include <stdlib.h>
#include <stdio.h>
#include <glib-2.0/glib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* Includes for retrieving internet interface parameters */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

/* to get the network interfaces*/
#include <ifaddrs.h>

/* header file */
#include "../include/deviceInfo/ethernetIfTable.h"

/**
 * \brief set the static IP given in parameter to the network interface
 * \param interface the name of the ethernet interface we want to set the IP
 * \param the new static IP to set
 * \return TRUE on SUCCES, FALSE on FAILURE
 */
static gboolean set_static_ip( const gchar *interface, const gchar *ip){

	struct ifreq ifr;
	struct sockaddr_in sai;
	int sockfd; /* socket fd we use to manipulate stuff with */

	/* Create a channel to the NET kernel. */
	sockfd = socket(AF_INET, SOCK_DGRAM , IPPROTO_UDP);

	/* get interface name */
	strncpy ( ifr.ifr_name , interface, IFNAMSIZ );

	/* set socket address members' values */
	memset(&sai, 0, sizeof(struct sockaddr));
	sai.sin_family = AF_INET;
	sai.sin_port = 0;

	/* get IP, store it in a u_int32t integer */
	sai.sin_addr.s_addr = inet_addr(ip);

	memcpy ( (struct sockaddr_in *)&ifr.ifr_addr , &sai, sizeof(struct sockaddr_in));

#if 0
	p = (char *) &sai;
	memcpy( (((char *)&ifr + ifreq_offsetof(ifr_addr) )),
			p, sizeof(struct sockaddr));
#endif

	/* set addresse sai to ifreq */

	if ( ioctl(sockfd, SIOCSIFADDR, &ifr) < 0 ){
		g_printerr("ERROR when trying to set static IP %s on %s: %s\n", ip, interface, strerror ( errno ));
		return FALSE;
	}

	if ( ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0 ) {
		g_printerr("ERROR when trying to set flags IP to %s: %s\n", interface, strerror ( errno ));
		return FALSE;
	}

	/*
	 * Now set the interface flugs: it must be at least UP and RUNNING
	 */
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;

	if ( ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0 ) {
		g_printerr("ERROR when trying to set flags IP to %s: %s\n", interface, strerror ( errno ));
		return FALSE;
	}

	/*
	 * close socket
	 */
	close(sockfd);

	return TRUE;

}

/**
 * \brief detect if there is IP any conflict in the VIVEO network
 */
static gboolean detect_confilct(){}




