/*
 * Licence: GPL
 * Created: Fri, 26 Feb 2016 10:53:53 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef SDP_H
#define SDP_H

gboolean create_SDP(GstSDPMessage 	*msg, struct channelTable_entry * channel_entry);
GstCaps* get_SDP(unsigned char *array, int sdp_msg_size, in_addr_t *multicast_addr);

#endif /* SDP_H */

