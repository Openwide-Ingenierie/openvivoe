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
#include "../include/deviceInfo/ethernetIfTable.h"

/** \brief Retrieve parameters of a specific interface used in VIVOE MIB
 *  @param iface The name of  network interface to use to retrieve the IP
 *  @param ifr a structure to store the interface information
 *  @param param The int parameter that we want to retireve. It corespond to the number to pass to ioctl
 *  @return TRUE if we succeed to get interface info on param, FALSE otherwise
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
		g_printerr("ERROR: get_ip(): ioctl failed with error message \"%s\"\n", strerror(errno));
		return FALSE;
	}
	return TRUE;
}

i/** \brief List all network interface using IPv4
 *  @param if_names the array of string to store the result
 *  @return TRUE if we succeed to list IPv4 interface FALSE otherwise
 */
static gboolean list_interfaces(char** if_names){
		/* Get all interfaces */
	struct ifaddrs 	*ifap;
	struct ifaddrs 	*iterator;
	int 			i = 0; /* array accessor variable */
    if( getifaddrs(&ifap) < 0){
		printf("ERROR: Failed to get system's network interfaces\n");
		return FALSE;
	}
	/* listing interfaces */
	g_print("Listing network interfaces using IPv4...\n");
	/* iterate through interfaces */ 
	for (iterator = ifap; iterator; iterator = iterator->ifa_next) {
		/* get only IPv4 interface */
        if (iterator->ifa_addr->sa_family==AF_INET)
		{
			if_names 	= (char**) 	realloc(if_names, (i+1) * sizeof(char*));
			if_names[i] = (char*) 	malloc( (strlen( iterator->ifa_name ) +1) * sizeof(char));
			strcpy(if_names[i], iterator->ifa_name);
			i++;
		}
	}
	if(if_names)
		return TRUE;
	else
		return FALSE;
}


/** \brief Ask the user which interface he wants VIVOE to use
 *  @param if_names the list of network interface comptable for VIVOE
 *  @return TRUE if we succeed to list IPv4 interface FALSE otherwise
 */

/* define the number of ioctl calls to make */ 
#define calls_num 		5		
/** \brief Retrieve parameters of ethernet interface and initiate the MIB with it
 * @paramiface the interface name from which retirev
 */
gboolean init_ethernet(const char* iface){

	/* the structure to use for retrieving information */
	struct 		ifreq ifr;
	long  		ethernetIfIndex;
	long 		ethernetIfSpeed;
	u_char 		ethernetIfMacAddress[MAC_ADDRESS_SIZE];
	size_t 		ethernetIfMacAddress_len = MAC_ADDRESS_SIZE;
	in_addr_t 	ethernetIfIpAddress;
	in_addr_t 	ethernetIfSubnetMask;
	in_addr_t 	ethernetIfIpAddressConflict;
	/* A array with the different call to ioctl to make */ 
	int ioctl_call[calls_num] = {SIOCGIFINDEX,SIOCGIFMTU, SIOCGIFHWADDR, SIOCGIFADDR, SIOCGIFNETMASK };
	int i; /* loop variable */
	/* Get interface IP Address */
   for(i = 0; i<calls_num; i++){
		if( get_interface_info(iface, &ifr, ioctl_call[i])){
			switch(ioctl_call[i]){
				case SIOCGIFINDEX:
					ethernetIfIndex 		= (ifr.ifr_ifindex);					
					break;
				case SIOCGIFMTU:
					ethernetIfSpeed 		= (ifr.ifr_mtu);		
					break;
				case SIOCGIFHWADDR:
					memcpy(ethernetIfMacAddress , (unsigned char*) ifr.ifr_hwaddr.sa_data, ethernetIfMacAddress_len);
					break;
				case SIOCGIFADDR:
					ethernetIfIpAddress 	= inet_netof(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
					break;
				case SIOCGIFNETMASK:
					ethernetIfSubnetMask 	= inet_netof(((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr);
					break;
				default:
					/* this is a really bad error, we should never get there */
					g_printerr("ERROR: unknown ioctl call\n");
					return FALSE;
			}
		}else{
			g_printerr("ERROR: Failed to retrieve parameters of %s\n", iface);
			return FALSE;
		}
	}
	/* now we can create an entry in the ethernetIfTable */
	/* create an null Ip conflict */
	ethernetIfIpAddressConflict = inet_netof(inet_makeaddr (ethernetIfSubnetMask&ethernetIfIpAddress , inet_addr("0.0.0.0")));
/*	struct ethernetIfTableEntry * new_entry =  ethernetIfTableEntry_create( ethernetIfIndex,
                                                          				  	ethernetIfSpeed,
			                                                           		ethernetIfMacAddress,
            			                                              		ethernetIfMacAddress_len,
			                                                          		ethernetIfIpAddress,
			                                                          		ethernetIfSubnetMask,
			                                                          		ethernetIfIpAddressConflict);*/
	return TRUE;
}
