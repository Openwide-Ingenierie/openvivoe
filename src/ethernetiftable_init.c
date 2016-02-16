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

/* header file */
#include "../include/deviceInfo/ethernetIfTable.h"

/** \brief Retrieve system IP address on a specific interface
 *  @param iface The network interface to use to retrieve the IP
 *  @param ifr a structure to store the interface information
 */
static gboolean get_interface_info(const char* iface, struct ifreq* ifr){
	int fd;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
    /* IPv4 */
    ifr->ifr_addr.sa_family = AF_INET;

    /* Specify network interface */
    strncpy(ifr->ifr_name, iface, IFNAMSIZ-1);

	/* 
	 * Now that we should have normally retrieve the information of the internet interface
	 * we need to check that we do have everything that we need to initiate the MIB
	 */

	/* ioctl - MAC address*/
	if(ioctl(fd, SIOCGIFHWADDR, ifr) < 0) {
		g_printerr("ERROR: get_ip(): ioctl failed with error message \"%s\"\n", strerror(errno));
		return FALSE;
	}
	/* ioctl - SubnetMask*/
	if(ioctl(fd, SIOCGIFNETMASK, ifr) < 0) {
		g_printerr("ERROR: get_ip(): ioctl failed with error message \"%s\"\n", strerror(errno));
		return FALSE;
	}
	/* ioctl - IP address*/
	if(ioctl(fd, SIOCGIFADDR, ifr) < 0) {
		g_printerr("ERROR: get_ip(): ioctl failed with error message \"%s\"\n", strerror(errno));
		return FALSE;
	}
	/* ioctl - Speed or MTU (Maximum Transert Unit*/
	if(ioctl(fd, SIOCGIFMTU, ifr) < 0) {
		g_printerr("ERROR: get_ip(): ioctl failed with error message \"%s\"\n", strerror(errno));
		return FALSE;
	}
	/* close socket */
    close(fd);
	return TRUE;
}


/** \brief Retrieve parameters of ethernet interface and initiate the MIB with it
 * @paramiface the interface name from which retirev
 */
gboolean init_ethernet(const char* iface){

	struct 		ifreq ifr;
	long  		ethernetIfIndex;
	long 		ethernetIfSpeed;
	//	u_char 		ethernetIfMacAddress[6];
	//	size_t 		ethernetIfMacAddress_len;
	in_addr_t 	ethernetIfIpAddress;
	in_addr_t 	ethernetIfSubnetMask;
	//	in_addr_t 	ethernetIfIpAddressConflict;
	//
	//
	if( get_interface_info(iface, &ifr)){
		printf("ethernetIfIpAddress: %s\n",  inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)	);	
		printf("ethernetIfSubnetMask: %s\n",  inet_ntoa(((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr)	 );		
		ethernetIfIndex 		= ((struct sockaddr_in *)&ifr.ifr_ifindex);
		ethernetIfSpeed 		= ((struct sockaddr_in *)&ifr.ifr_mtu);
		ethernetIfIpAddress 	= inet_netof(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
		ethernetIfSubnetMask 	= inet_netof(((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr);
		printf("ethernetIfIndex: %ld\n",  		ethernetIfIndex);
		printf("ethernetIfSpeed: %ld\n",  		ethernetIfSpeed);
		printf("ethernetIfSubnetMask: %8X\n", 	ethernetIfSubnetMask );
		return TRUE;
	}else{
		g_printerr("ERROR: Failed to retrieve parameters of %s\n", iface);
		return FALSE;
	}
}
