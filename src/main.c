/*
 * Licence: GPL
 * Created: Thu, 14 Jan 2016 12:05:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */

/* 
 * SubAgent Module
 */
#include <stdio.h>
#include "../include/deamon.h"
#include "../include/streaming/stream.h"
#include "../include/multicast.h"

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

int main (int   argc,  char *argv[]){
//	return stream(argc, argv);
//long mu;
//define_vivoe_multicast(&mu, "lo", 1);
//return 0; "enp2s0"
	return init_ethernet("enp2s0");
//	return deamon(argv[0]);
}

