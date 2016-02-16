/*
 * Licence: GPL
 * Created: Tue, 16 Feb 2016 14:07:57 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 *     - Gerome <gerome.burlats@openwide.fr>
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

#define DEFAULT_MULTICAST_PORT		5004
#define DEFAULT_MULTICAST_CHANNEL	"1"
#define DEFAULT_MULTICAST_ADDR		"239.192.1.254"
#define DEFAULT_MULTICAST_IFACE		"enp2s0"


/** \brief Retrieve system IP address on a specific interface
 *  @param iface The network interface to use to retrieve the IP
 *  @param ifr a structure to store the interface information
 */
static gboolean get_ip(const char* iface, struct ifreq* ifr){
	int fd;
	fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* IPv4 */
    ifr->ifr_addr.sa_family = AF_INET;

    /* Specify network interface */
    strncpy(ifr->ifr_name, iface, IFNAMSIZ-1);

	/* ioctl */
	if(ioctl(fd, SIOCGIFADDR, ifr) < 0) {
		g_printerr("ERROR: get_ip(): ioctl failed with error message \"%s\"\n", strerror(errno));
		return FALSE;
	}

    close(fd);
	return TRUE;
}

/** \brief Retrieve the device IP associated to the address addr
 * @param addr The addresse to use
 * @return the deviceID associated (last byte of IP address)
 */
static short get_device_id(char* addr){
	long int_addr 	= htonl(inet_addr( (const char*) addr)); /* get the addresse in binary mode, in network style (as host style may changed according to host) */
	long mask 		= 255 ; /* create the mask: 0x00.00.00.FF or 0x00.00.00.00.00.00.00.FF depending of architecture*/
	/* return the device ID */
	return (int_addr&mask);
}

/** \brief Build Multicast address for streaming in VIVOE protocol
 * @param multicast_addr the string to store the computed multicast address
 * @param multicast_iface the interface from which retirevinf the IP address
 * @param multicast_channel the multicast channel we wanna use
 */
 void define_vivoe_multicast(long *multicast_addr, const char* multicast_iface, const short int multicast_channel)
{
	char* 	device_ip 	= NULL;
	short 	device_id;
	struct 	ifreq ifr;
	/* constant values needed to build multicast address */ 
	long multicast_pref = htonl(inet_addr("239.192.0.0"));
	long channel 		= (long) (multicast_channel << 8);
	long result; /* the multicast_addr in a binary form */ 
		
	if( get_ip(multicast_iface, &ifr)){
		device_ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
		printf("%s\n", device_ip);
		device_id = get_device_id(device_ip);
		result = multicast_pref + channel + device_id;
		*multicast_addr =  result;
	} else{
		g_printerr("ERROR: Failed to retrieve IP on %s\n", multicast_iface);
	}
	if(!multicast_addr){
		*multicast_addr = htonl(inet_addr( DEFAULT_MULTICAST_ADDR ));
		printf("Failed to generate multicast IP for udpsink, using default address: %s\n", DEFAULT_MULTICAST_ADDR );
	}
}
