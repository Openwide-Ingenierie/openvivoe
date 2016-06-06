/*
 * Licence: GPL
 * Created: Tue, 31 May 2016 14:28:05 +0200
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
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <net/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h> /* the L2 protocols */

/* to get the network interfaces*/
#include <ifaddrs.h>

/* header file */
#include "../../include/deviceInfo/ethernetIfTable.h"
#include "../../include/vivoe_ip/ip_assignment.h"


#define IP_ADDR_LEN  			4
#define ETH_BRD_ADD_BYTE  		0xFF
#define ETH_PROBE_ADRRESS_BYTE 	0x00
#define ETH_FRAME_SIZE 			1024

#define DEFAULT_DEVICE "eth0"

/**
 * \brief ARP header structure
 */
struct  __attribute__((packed)) arp_packet {
	struct arphdr 		arp_hdr; /* ARP header as defined in kernel */
	/* extrat fields These are not compiled in the arphdr structure. I don't konw why */
	uint8_t 	arp_sha[6]; /* Sender Hardware Address */
	uint8_t 	arp_spa[4]; /* Sender Protocol Address*/
	uint8_t 	arp_tha[6]; /* Target Hardware Address */
	uint8_t 	arp_tpa[4]; /* Target Protocol Address */
};

static void build_arp_probe_packet(struct ethernetIfTableEntry *if_entry, struct arp_packet *pkt ){

	/*
	 * Hardware Address is set to Ethernet 10/100Mbps
	 * See if_arp.h for detail
	 */
	pkt->arp_hdr.ar_hrd = htons(ARPHRD_ETHER);

	/*
	 * Format of protocol address is 0x8000 for IPv4
	 */
	pkt->arp_hdr.ar_pro = htons(ETHERTYPE_IP);

	/*
	 * length of hardware addr
	 */
	pkt->arp_hdr.ar_hln = htons(if_entry->ethernetIfMacAddress_len);

	/*
	 * length of hardware addr
	 */
	pkt->arp_hdr.ar_pln = htons(IP_ADDR_LEN);

	/*
	 * In VIVOE we only sent ARP Request
	 */
	pkt->arp_hdr.ar_op = htons(ARPOP_REQUEST);

	/*
	 * Set ARP Sender Hardware Address
	 */
	memcpy( pkt->arp_sha, if_entry->ethernetIfMacAddress , if_entry->ethernetIfMacAddress_len );

	/*
	 * In ARP probe packet, sender IP address is 0
	 */
	memset( pkt->arp_spa, ETH_PROBE_ADRRESS_BYTE , IP_ADDR_LEN * sizeof (uint8_t) );

	/*
	 * Set the Target Hardware Address: all 0 in ARP Probe
	 */
	memset (pkt->arp_tha , ETH_PROBE_ADRRESS_BYTE , ETHER_ADDR_LEN * sizeof (uint8_t));

	/*
	 * Set the Target IP Address: in a Probe Message this is the IP address being probed i.e. IP address statically assigned to the device
	 */
	memset (pkt->arp_tpa , if_entry->ethernetIfIpAddress , IP_ADDR_LEN  * sizeof (uint8_t));

}

static void build_arp_anouncement_packet(struct ethernetIfTableEntry *if_entry, struct arp_packet *pkt ){

	/*
	 * Hardware Address is set to Ethernet 10/100Mbps
	 * See if_arp.h for detail
	 */
	pkt->arp_hdr.ar_hrd = htons(ARPHRD_ETHER);

	/*
	 * Format of protocol address is 0x8000 for IPv4
	 */
	pkt->arp_hdr.ar_pro = htons(ETHERTYPE_IP);

	/*
	 * length of hardware addr
	 */
	pkt->arp_hdr.ar_hln = htons(if_entry->ethernetIfMacAddress_len);

	/*
	 * length of hardware addr
	 */
	pkt->arp_hdr.ar_pln = htons(IP_ADDR_LEN);

	/*
	 * In VIVOE we only sent ARP Request
	 */
	pkt->arp_hdr.ar_op = htons(ARPOP_REQUEST);

	/*
	 * Set ARP Sender Hardware Address
	 */
	memcpy( pkt->arp_sha, if_entry->ethernetIfMacAddress , if_entry->ethernetIfMacAddress_len );

	/*
	 * In ARP anouncment packet, sender IP address
	 */
	memset( pkt->arp_spa, if_entry->ethernetIfIpAddress , IP_ADDR_LEN * sizeof (uint8_t) );

	/*
	 * Set the Target Hardware Address: all 0 in ARP Probe
	 */
	memset (pkt->arp_tha , ETH_PROBE_ADRRESS_BYTE , ETHER_ADDR_LEN * sizeof (uint8_t));

	/*
	 * Set the Target IP Address: in a anouncement Message this is the IP address being used
	 */
	memset (pkt->arp_tpa , if_entry->ethernetIfIpAddress , IP_ADDR_LEN  * sizeof (uint8_t));

}


gboolean send_arp_packet( struct ethernetIfTableEntry *if_entry , gboolean probe ) {

	struct arp_packet pkt;
	struct sockaddr_ll sa;
	int socket_fd;
	uint8_t eth_frame[ETH_FRAME_SIZE];
	struct ether_header *eh =  (struct ether_header *) eth_frame;
	int eth_header_length = sizeof( struct ether_header);
	int eth_frame_length = 0;
	int bytes; /* number of bytes send */

	if ( !if_entry){
		g_critical("ethernetIfTableEntry provided is NULL");
		return FALSE;
	}

	/* field Ethernet Header */
	memset( eh->ether_dhost , ETH_BRD_ADD_BYTE , ETHER_ADDR_LEN ); /* destination eth address */
	memcpy( eh->ether_shost , if_entry->ethernetIfMacAddress, if_entry->ethernetIfMacAddress_len); /* source ethernet address */
	eh->ether_type =  htons( ETH_P_ARP ); /* Next is ethernet type code (ETH_P_ARP for ARP) */

	/* copy ethernet header in ethernet frame */
	memcpy(eth_frame, eh, eth_header_length );

	/* Build ARP packet to send */
	if ( probe )
		build_arp_probe_packet ( if_entry, &pkt);
	else
		build_arp_anouncement_packet ( if_entry, &pkt);

	/* copy ARP packet in ETHERNET payload */
	memcpy( eth_frame + eth_header_length, &pkt , sizeof( struct arp_packet ));

	/* open the socket */
	socket_fd = socket( AF_PACKET , SOCK_RAW , IPPROTO_RAW ) ;
	if ( socket_fd < 0 ){
		g_critical("send_arp_packet(): cannot open socket: %s", strerror(errno));
		return FALSE;
	}

	/* build socket information */
	sa.sll_ifindex = if_entry->ethernetIfIndex;
	/* Address length*/
	sa.sll_halen = ETH_ALEN;
	/* Destination MAC */
	memset( sa.sll_addr , ETH_BRD_ADD_BYTE , ETHER_ADDR_LEN );

	/* Send ethernet frame to socket.*/
	if ((bytes = sendto (socket_fd , eth_frame , ETH_FRAME_SIZE , 0, (struct sockaddr *) &sa, sizeof (sa))) <= 0) {
		g_critical("send_arp_packet(): failed to sent ARP packet");
		return FALSE;
	}

	return TRUE;

}

gboolean ip_conflict_detection(  struct ethernetIfTableEntry *if_entry ){
	/* send ARP PROBE */

	/* wait ANOUNCEMENT_WAIT: if no reply, send it again */

	/* check if conflict */

	/* if so, send TRAP, select random IP and restart */

	/* otherwise send ARP ANOUNCEMENT */
}
