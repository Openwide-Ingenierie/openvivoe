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

/* Includes for retrieving and setting internet interface parameters */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/types.h>
#include <arpa/inet.h>

/* to get the network interfaces*/
#include <ifaddrs.h>

/* header file */
#include "../../include/mibParameters.h"
#include "../../include/deviceInfo/ethernetIfTable.h"
#include "../../include/vivoe_ip/ip_assignment.h"




/**
 * \brief set the static IP given in parameter to the network interface
 * \param interface the name of the ethernet interface we want to set the IP
 * \param the new static IP to set
 * \return TRUE on SUCCES, FALSE on FAILURE
 */
gboolean set_static_ip( const gchar *interface, const gchar *ip){

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

	/* set addresse sai to ifreq */
	memcpy ( (struct sockaddr_in *)&ifr.ifr_addr , &sai, sizeof(struct sockaddr_in));

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

static int ip_tested = RANDOM_MIN_SUFFIX;

/**
 * \brief gives a random IP selected between 192.168.204.200 and 192.168.204.253
 * \param interface the name of the ethernet interface we are working on
 * \return a random IP selected between 192.168.204.200 and 192.168.204.253
 */
in_addr_t random_ip_for_conflict( gchar *interface)
{

	struct in_addr 	random_ip;
	struct in_addr 	prefix; /* 192.168.204.0 */
	int 			random_suffix;

	prefix.s_addr = inet_addr ( RANDOM_IP_PREFIX ) ;

	if ( ip_tested != RANDOM_MAX_SUFFIX + 1){
		random_suffix = ip_tested;
		ip_tested ++;
	}
	else{
		random_suffix = RANDOM_MAX_SUFFIX + 1;
	}

	/* add suffix to generated IP */
	random_ip.s_addr = prefix.s_addr + htonl(random_suffix) ;

	/* assigned static IP */
	g_debug("random_ip_for_conflict(): assigned new IP to device: %s", inet_ntoa(random_ip));
	set_static_ip( interface , inet_ntoa(random_ip) );

	return random_ip.s_addr;
}

/**
 * \brief assgined default static IP 192.168.204.254
 * \param interface the name of the ethernet interface we are working on
 */
gboolean assign_default_ip( const gchar *interface){

	g_debug( "assign_default_ip()");

	return set_static_ip( interface , DEFAULT_STATIC_IP );

}
