/*
 * Licence: GPL
 * Created: Tue, 16 Feb 2016 17:26:22 +0100
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
#include "deviceInfo/ethernetIfTable.h"

/** \brief Retrieve parameters of a specific interface used in VIVOE MIB
 *  \param iface The name of  network interface to use to retrieve the IP
 *  \param ifr a structure to store the interface information
 *  \param param The int parameter that we want to retireve. It corespond to the number to pass to ioctl
 *  \return TRUE if we succeed to get interface info on param, FALSE otherwise
 */
static gboolean get_interface_info(const char* iface, struct ifreq* ifr, int param){
	int fd;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
    /* IPv4 */
    ifr->ifr_addr.sa_family = AF_INET;

	memset(ifr, 0, sizeof(*ifr));
    /* Specify network interface */
    strncpy(ifr->ifr_name, iface, IFNAMSIZ-1);

	/* ioctl - retrieve parameter from interface*/
	if( ioctl(fd, param, ifr) < 0) {
		g_critical("ERROR: get_ip(): ioctl failed with error message \"%s\"", strerror(errno));
		return FALSE;
	}
	return TRUE;
}

/* define the number of ioctl calls to make */
#define calls_num 		4
/**
 * \brief Retrieve parameters of ethernet interface and initiate the MIB with it
 * \param iface the interface name from which retirev
 * \return the entry created NULL otherwise
 */
struct ethernetIfTableEntry *init_ethernet(const char* iface){

	/* the structure to use for retrieving information */
	struct 		ifreq ifr;
	long 		ethernetIfSpeed 							= 0;
	u_char 		ethernetIfMacAddress[MAC_ADDRESS_SIZE];
	size_t 		ethernetIfMacAddress_len 					= MAC_ADDRESS_SIZE;
	in_addr_t 	ethernetIfIpAddress 						= 0;
	in_addr_t 	ethernetIfSubnetMask 						= 0;
	in_addr_t 	ethernetIfIpAddressConflict 				= 0;
	/* A array with the different call to ioctl to make */
	int ioctl_call[calls_num] = {SIOCGIFMTU, SIOCGIFHWADDR, SIOCGIFADDR, SIOCGIFNETMASK };
	int i; /* loop variable */
	/* Get interface IP Address */
   for(i = 0; i<calls_num; i++){
		if( get_interface_info(iface, &ifr, ioctl_call[i])){
			switch(ioctl_call[i]){
				case SIOCGIFMTU:
					ethernetIfSpeed 		= (ifr.ifr_mtu);
					break;
				case SIOCGIFHWADDR:
					memcpy(ethernetIfMacAddress , (unsigned char*) ifr.ifr_hwaddr.sa_data, ethernetIfMacAddress_len);
					break;
				case SIOCGIFADDR:
					ethernetIfIpAddress 	= ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
					break;
				case SIOCGIFNETMASK:
					ethernetIfSubnetMask 	= ((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr.s_addr;
					break;
				default:
					/* this is a really bad error, we should never get there */
					g_critical("unknown ioctl call");
					return NULL;
			}
		}else{
			g_critical("Failed to retrieve parameters of %s", iface);
			return NULL;
		}
	}
	/* now we can create an entry in the ethernetIfTable */
	/* create an null Ip conflict */
	ethernetIfIpAddressConflict = inet_netof(inet_makeaddr (0 , inet_addr("0.0.0.0")));
	return ethernetIfTableEntry_create( ethernetIfSpeed,
			                     ethernetIfMacAddress,
            			         ethernetIfMacAddress_len,
			                     ethernetIfIpAddress,
			                     ethernetIfSubnetMask,
			                     ethernetIfIpAddressConflict);

}
