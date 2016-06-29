/*
 * Licence: GPL
 * Created: Tue, 16 Feb 2016 14:07:57 +0100
 * Main authors:
 *     - hoel <hoel.vasseur\openwide.fr>
 *     - Gerome <gerome.burlats\openwide.fr>
 */

#include <stdlib.h>
#include <stdio.h>
#include <glib-2.0/glib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Includes for retrieving IP addr */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* header file */
#include "include/deviceInfo/ethernetIfTable.h"
#include "include/mibParameters.h"
#include "include/multicast.h"

/**
 * \brief Retrieve the device ID associated to the address ip_ddr
 * \param ip_addr The addresse to use
 * \return the deviceID associated (last byte of IP address)
 */
static uint8_t get_device_id(struct in_addr ip_addr){
	long mask 		= 0xFF ; /* create the mask: 0x00.00.00.FF or 0x00.00.00.00.00.00.00.FF depending of architecture*/
	/* return the device ID */
	return (htonl(ip_addr.s_addr)&mask);
}

/**
 * \brief Build Multicast address for streaming in VIVOE protocol
 * \param multicast_addr the string to store the computed multicast address
 * \param multicast_iface the interface from which retirevinf the IP address
 * \param multicast_channel the multicast channel we wanna use
 * \return the multicast_addr in a long form
 */
long define_vivoe_multicast( struct ethernetIfTableEntry *if_entry, const short int multicast_channel)
{
	short 	device_id;
	struct in_addr ip_addr;
	/* constant values needed to build multicast address */
	unsigned long multicast_pref = ntohl(inet_addr("239.192.0.0"));
	unsigned long channel 		= (long) (multicast_channel << 8);
	unsigned long result = -1; /* the multicast_addr in a binary form */

	g_debug("define_vivoe_multicast: compute multicast IP address for channel %d", multicast_channel);

	if ( if_entry ){

		/* save ip in ip_addr */
		ip_addr.s_addr = if_entry->ethernetIfIpAddress;
		device_id = get_device_id(ip_addr);
		result = multicast_pref + channel + device_id;

	} else{

		g_error("ERROR: Failed to retrieve IP on %s", get_primary_interface_name() );

	}

	if(result < 0){

		g_critical("Failed to generate multicast IP for udpsink, using default address: %s", DEFAULT_MULTICAST_ADDR );
		return ntohl(inet_addr( DEFAULT_MULTICAST_ADDR ));

	}

	return ntohl(result);
}
