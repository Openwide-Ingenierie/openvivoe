/*
 * Licence: GPL
 * Created: Tue, 16 Feb 2016 14:08:06 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef MULTICAST_H
# define MULTICAST_H

/* defines default parameters */
#define DEFAULT_MULTICAST_PORT		5004
#define DEFAULT_MULTICAST_CHANNEL	"1"
#define DEFAULT_MULTICAST_ADDR		"239.192.1.254"
#define DEFAULT_MULTICAST_IFACE		"enp2s0"
long define_vivoe_multicast(const char* multicast_iface, const short int multicast_channel);

#endif /* MULTICAST_H */

