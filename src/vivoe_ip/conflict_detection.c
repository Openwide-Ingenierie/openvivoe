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
#include <netinet/ip.h>       /* IP_MAXPACKET (65535) */
#include <time.h>

/* to get the network interfaces*/
#include <ifaddrs.h>

/* header file */
#include "../../include/trap/ipAddressConflict.h"
#include "../../include/vivoe_ip/ip_assignment.h"
#include "../../include/deviceInfo/ethernetIfTable.h"

#define IP_ADDR_LEN  			4
#define ETH_BRD_ADD_BYTE  		0xFF
#define ETH_PROBE_ADRRESS_BYTE 	0x00


/* RFC 5527 values */
#define PROBE_WAIT 				1 /* second   (initial random delay) */
#define PROBE_NUM 				3 /* (number of probe packets) */
#define PROBE_MIN 				1 /* second   (minimum delay until repeated probe) */
#define PROBE_MAX 				2 /* seconds  (maximum delay until repeated probe) */
#define ANNOUNCE_WAIT 			2 /* seconds  (delay before announcing) */
#define ANNOUNCE_NUM 			2 /* (number of Announcement packets) */
#define ANNOUNCE_INTERVAL 		2 /* seconds  (time between Announcement packets) */
#define MAX_CONFLICTS 			10 /* (max conflicts before rate-limiting) */
#define RATE_LIMIT_INTERVAL 	60 /* seconds  (delay between successive attempts) */
#define DEFEND_INTERVAL 		10 /* seconds  (minimum interval between defensive */

/**
 * \brief ARP header structure
 */
struct  __attribute__((packed)) arp_packet {
	struct arphdr 		arp_hdr; /* ARP header as defined in kernel */
	/* extrat fields These are not compiled in the arphdr structure. I don't konw why */
	uint8_t 			arp_sha[6]; /* Sender Hardware Address */
	uint8_t 			arp_spa[4]; /* Sender Protocol Address*/
	uint8_t 			arp_tha[6]; /* Target Hardware Address */
	uint8_t 			arp_tpa[4]; /* Target Protocol Address */
};

/**
 * \brief buil ARP Probe Message with IP of sender extract from MIB
 * \param if_entry the MIB entry of teh ethernet interface
 * \param pkt the ARP packet to fill
 */
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
	pkt->arp_hdr.ar_hln = if_entry->ethernetIfMacAddress_len;

	/*
	 * length of hardware addr
	 */
	pkt->arp_hdr.ar_pln = IP_ADDR_LEN;

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
	memcpy (pkt->arp_tpa , &if_entry->ethernetIfIpAddress , IP_ADDR_LEN  * sizeof (uint8_t));

}

/**
 * \brief buil ARP ANOUNCEMENT Message with IP of sender extract from MIB
 * \param if_entry the MIB entry of teh ethernet interface
 * \param pkt the ARP packet to fill
 */
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
	pkt->arp_hdr.ar_hln = if_entry->ethernetIfMacAddress_len;

	/*
	 * length of hardware addr
	 */
	pkt->arp_hdr.ar_pln = IP_ADDR_LEN;

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
	memcpy (pkt->arp_tpa , &if_entry->ethernetIfIpAddress , IP_ADDR_LEN  * sizeof (uint8_t));

}

/**
 * \brief send an ARP request: PROBE msg or ANOUNCE msg
 * \param if_entry the MIB entry of the ethernet interface
 * \param probe specify the kind of ARP packet to send TRUE for PROBE, FALSE for ANOUNCE
 * \return TRUE if the request could be sent, FALSE otherwise
 */
gboolean send_arp_request( struct ethernetIfTableEntry *if_entry , gboolean probe ) {

	struct arp_packet pkt;
	struct sockaddr_ll sa;
	int socket_fd;
	int eth_header_length = sizeof( struct ether_header);
	int eth_frame_length =  ( eth_header_length + sizeof( struct arp_packet ) );
	uint8_t *eth_frame	= malloc ( eth_frame_length * sizeof ( uint8_t));
	struct ether_header *eh =  (struct ether_header *) eth_frame;
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
	sa.sll_halen = htons( ETH_ALEN ) ;
	/* Destination MAC */
	memset( sa.sll_addr , ETH_BRD_ADD_BYTE , ETHER_ADDR_LEN );

	/* Send ethernet frame to socket.*/
	if ((bytes = sendto (socket_fd , eth_frame , eth_frame_length , 0, (struct sockaddr *) &sa, sizeof (sa))) <= 0) {
		g_critical("send_arp_packet(): failed to sent ARP packet");
		return FALSE;
	}

	/* close the socket */
	close ( socket_fd ) ;

	return TRUE;

}

/**
 * \brief receive an ARP message, check if it is our reply
 * \return TRUE if this is a reply to our previous PROBE message, and s if there is a conflict, FALSE otherwise
 */
static gboolean receive_arp_reply(  ) {

	int sd, status;
	uint8_t *ether_frame;
	struct ether_header *eh;
	struct arp_packet *arp_pkt;

	g_debug("receive_arp_packet()");

	/* Allocate memory for various arrays. */
	ether_frame = malloc (IP_MAXPACKET);

	/* Submit request for a raw socket descriptor. */
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		g_critical ("receive_arp_packet(): cannot open socket: %s", strerror(errno));
		return FALSE;
	}

	/* Listen for incoming ethernet frame from socket sd.
	 * We expect an ARP ethernet frame of the form:
	 * MAC souce (6 bytes) + MAC destination (6 bytes) + ethernet type (2 bytes) + ethernet data (ARP header) (28 bytes)
	 * Keep at it until we get an ARP reply.
	 */
	arp_pkt = (struct arp_packet *) (ether_frame + sizeof (struct arp_packet) );
	eh 		= (struct  ether_header*) ether_frame;

	GTimer *timer = g_timer_new();
	while ((ntohs (arp_pkt->arp_hdr.ar_op) != ETH_P_ARP) && (ntohs (arp_pkt->arp_hdr.ar_op) != ARPOP_REPLY)) {

		if ((status  = recvfrom(sd , ether_frame ,IP_MAXPACKET , 0, NULL, NULL)) < 0) {
			if (errno == EINTR) {
				g_debug("receive_arp_packet(): we have been interrupted while receiving ARP reponse");
				memset (ether_frame, 0, IP_MAXPACKET * sizeof (uint8_t));
				continue;  /* Something weird happened, but let's try again. */
			} else {
				g_critical (" receive_arp_packet() failed");
				return FALSE;
			}
		}

		/* get received ethernet frame header */
		memcpy ( eh , ether_frame , sizeof ( struct ether_header ));
		/* We have received a Ethernet Packet, check if this is our ARP reply */
		if ( ntohs ( eh->ether_type ) == ETH_P_ARP){
			/* check if this is not the old ARP packet */
			/*copy ARP packet into arp_pkt */
			memcpy( arp_pkt, ether_frame + 14, sizeof (struct arp_packet));
			if ( (ntohs (arp_pkt->arp_hdr.ar_op) == ARPOP_REPLY)){
				/* response has been received, break */
				break;
			}
		}

		/* check how many time we have wait, if ANNOUNCE_WAIT has been reach then return FALSE : no conflict */
		if ( g_timer_elapsed ( timer, NULL ) > ANNOUNCE_WAIT )
			return FALSE;

	}

	/* close the socket */
	close (sd);

	return TRUE;

}

/**
 * \brief detect and resolve IP conflict
 * \param the ethernet interface entry in the MIB for which we change the IP
 * \param interface the name of this interface (eth0/enp2s0/lo...)
 * \return TRUE if conflict could be resolved, FALSE otherwise
 */
gboolean ip_conflict_detection(  struct ethernetIfTableEntry *if_entry, gchar *interface ){

	g_debug ("ip_conflict_detection()" );

	/* select a random time to wait between 0 en PROBE_WAIT */
	srand(time(NULL));
	/* PROBE_WAIT is given in second, if we want a integer value for microsecond , we should multiply it by 1000000 */
	gdouble probe_wait =  ((gdouble) rand()/ (gdouble)(RAND_MAX)) * PROBE_WAIT;
	/* send PROB_NUM ARP PROBE messages between space randomly between PROBE_MIN and PROBE_MAX number*/
	gdouble space = (((gdouble) rand()/ (gdouble)(RAND_MAX)) /  (gdouble) ((PROBE_MAX - PROBE_MIN) * PROBE_NUM));
	gdouble time_passed = 0 ;
	gboolean conflict = TRUE ;
	in_addr_t max_ip_value = inet_addr ( DEFAULT_STATIC_IP ) ;

	GTimer *timer = g_timer_new();

	/* wait probe_wait */
	g_debug ("waiting %G second(s)", probe_wait);
	g_timer_start( timer );
	while(time_passed < probe_wait){
		time_passed = g_timer_elapsed ( timer, NULL );
	}

	while ( conflict ){
		/* wait PROBE_MIN */
		g_debug ("waiting %d second(s)", PROBE_MIN);
		g_timer_reset( timer );
		time_passed = g_timer_elapsed ( timer, NULL );
		while( time_passed  <  (PROBE_MIN) ){
			time_passed = g_timer_elapsed ( timer, NULL );
		}

		/* send ARP PROBE message PROBE_NUM times with an interval probe_wait */
		g_debug("send ARP Probe message %d times every %G second(s)", PROBE_NUM, space);
		for( int i = 1 ; i <= PROBE_NUM ; i ++){

			g_timer_reset( timer );
			time_passed = g_timer_elapsed ( timer, NULL );
			while( time_passed  <  space ){
				time_passed = g_timer_elapsed ( timer, NULL );
			}

			send_arp_request( if_entry , TRUE );

		}

		conflict = receive_arp_reply( ) ;

		/* if receive_arp_reply return FALSE there is a conflict, else, we are fine with this IP  */
		if ( conflict ){
			/* pick up a new random IP */
			if_entry->ethernetIfIpAddressConflict 	= if_entry->ethernetIfIpAddress ;
			if_entry->ethernetIfIpAddress 			= random_ip_for_conflict(interface);
			if ( if_entry->ethernetIfIpAddress == max_ip_value )
			   return TRUE;
			/* send trap */
			send_ipAddressConflict_trap();
		}else
			return FALSE;
	}

	return FALSE;

}
