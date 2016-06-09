/*
 * Licence: GPL
 * Created: Tue, 16 Feb 2016 14:08:06 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef MULTICAST_H
# define MULTICAST_H

//long define_vivoe_multicast(const char* multicast_iface, const short int multicast_channel);
long define_vivoe_multicast( struct ethernetIfTableEntry *if_entry, const short int multicast_channel);

#endif /* MULTICAST_H */

