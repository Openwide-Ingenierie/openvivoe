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
#include <net/ethernet.h> /* the L2 protocols */

/* to get the network interfaces*/
#include <ifaddrs.h>

/* header file */
#include "../../include/deviceInfo/ethernetIfTable.h"
#include "../../include/vivoe_ip/ip_assignment.h"


#define ETH_HW_ADDR_LEN 6
#define IP_ADDR_LEN     4
#define ETH_BRC_ADRRESS 0xff

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

#if 0

void die(char *);
void get_ip_addr(struct in_addr*,char*);
void get_hw_addr(char*,char*);
#endif

void build_arp_probe_packet(struct ethernetIfTableEntry *if_entry, struct arp_packet *pkt ){

	/*
	 * Hardware Address is set to Ethernet 10/100Mbps.
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
	memcpy( pkt->arp_spa, &(if_entry->ethernetIfIpAddress) , IP_ADDR_LEN );

	/*
	 * Set the Target  Address: the broadcast address
	 */
	memset (pkt->arp_tha , ETH_BRC_ADRRESS ,  ETH_HW_ADDR_LEN * sizeof (uint8_t));


}
#if 0
gboolean ip_conflict_detection( struct ethernetIfTableEntry *if_entry ) {

	struct in_addr src_in_addr, targ_in_addr;
	struct ether_header;
	struct arp_packet pkt;
	struct sockaddr sa;
	int socket_fd;

	/* opens the socket */
	socket_fd = socket( AF_INET, SOCK_PACKET, htons(ETH_P_RARP) ) ;
	if ( socket_fd < 0 ){
		g_critical("ERROR: ip_confilct_detection: cannot open socket\n");
		return FALSE;
	}

	// Submit request for a raw socket descriptor.
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		perror ("socket() failed ");
		exit (EXIT_FAILURE);
	}

	// Send ethernet frame to socket.
	if ((bytes = sendto (sd, ether_frame, frame_length, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) {
		perror ("sendto() failed");
		exit (EXIT_FAILURE);
	}




	return TRUE;
}


void die(char* str){
  fprintf(stderr,"%s\n",str);
  exit(1);
} ; // die

void get_ip_addr( struct in_addr* in_addr, char* str ){

  struct hostent *hostp;

  in_addr->s_addr = inet_addr(str);
  if ( in_addr->s_addr == -1 ){
    if( (hostp = gethostbyname(str)))
      bcopy( hostp->h_addr, in_addr, hostp->h_length ) ;
    else {
      fprintf( stderr, "send_arp: unknown host [%s].\n", str ) ;
      exit(1);
    }
  }
} ; // get_ip_addr

void get_hw_addr( char* buf, char* str ){

  int i;
  char c,val;

  for ( i=0 ; i < ETH_HW_ADDR_LEN ; i++ ){
    if( !(c = tolower(*str++))) die("Invalid hardware address");
    if(isdigit(c)) val = c-'0';
    else if(c >= 'a' && c <= 'f') val = c-'a'+10;
    else die("Invalid hardware address");

    *buf = val << 4;
    if( !(c = tolower(*str++))) die("Invalid hardware address");
    if(isdigit(c)) val = c-'0';
    else if(c >= 'a' && c <= 'f') val = c-'a'+10;
    else die("Invalid hardware address");

    *buf++ |= val;

    if (*str == ':') str++ ;
  } ; // for loop
} ; // get_hw_addr

#endif
