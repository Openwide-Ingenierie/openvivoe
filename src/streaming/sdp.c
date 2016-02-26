/*
 * Licence: GPL
 * Created: Fri, 26 Feb 2016 10:53:40 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/sdp/gstsdpmessage.h>
#include <arpa/inet.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* Header file */
#include "../../include/channelControl/channelTable.h"
#include "../../include/streaming/sdp.h"
#include "../../include/streaming/stream.h"

/** 
 * \brief Create the SDP message corresponding to the stream
 * \pramam strea_datas the stream_datas (pipeline, bus, bus_watch_id) associated to the video stream
 */
gboolean create_SDP(struct channelTable_entry * channel_entry ){

	GstSDPMessage 	*msg;
	/* create a new SDP message */
	if (gst_sdp_message_new(&msg)){
		g_printerr("Failed to create SDP message\n");
		return FALSE;
	}

	/* version : shall be set to 0 for VIVOE 
	 * v=0
	 */
	if (gst_sdp_message_set_version(msg, "0")){
		g_printerr("Failed to set version in SDP message\n");
		return FALSE;
	}
	/* origine:
	 * o=<username> <session id> <session version> <network type> <address type> <address> 
	 */
	/* build session id and session version using random numbers*/
	srand(time(NULL));
	int session_id 			= rand();
	int session_version 	= rand();

	char session_id_str[32];
	sprintf(session_id_str, "%d", session_id);

	char session_version_str[32];
	sprintf(session_version_str, "%d", session_version);

	/* get multicast IP from MIB */
	struct in_addr ip_addr;
    ip_addr.s_addr			= channel_entry->channelReceiveIpAddress;
	
	
	if ( !gst_sdp_address_is_multicast ("IN", "IPV4",inet_ntoa(ip_addr)) ){
		g_printerr("ERROR: try to stream on a non multicast IPv4 address\n");
	}
	gst_sdp_message_set_origin ( 	msg,
									"-" /* username*/,
									session_id_str /* sessionid*/,
									session_version_str/* sessionversion*/,
									"IN", /* network type :internet (always for VIVOE)*/
									"IPV4", /* address type :IPV4 (always for VIVOE)*/
									inet_ntoa(ip_addr) /* multicast IP*/
								); 
	

	printf("%s\n", gst_sdp_message_as_text(msg)); 
#if 0
		/* to see our work */
	//
gst_sdp_address_is_multicast ()


/
	/* session name
	 * s=<session name>
	 */
gst_sdp_message_set_session_name

/* connection:
 * c=<network type> <address type> <address>
 */
gst_sdp_message_set_connection ()

/* time : shall be set yo 0 0 in VIVOE to describe an unbound session
 *
 * t=<start time> <stop time>
 */
gst_sdp_message_add_time(

	/* media: 
	 * m=<media> <port> <transport> <fmt list>
	 */
gst_sdp_media_new ()
gst_sdp_media_set_port_info ()
gst_sdp_media_set_proto ()
gst_sdp_media_add_format ()
gst_sdp_media_set_information ()
gst_sdp_media_add_connection ()
	/* Add the media to SDP message */
	gst_sdp_message_add_media ()

/* a=rtpmap:<payload type> <encoding name>/<clock rate>	 */

#endif //if 0
	 return TRUE;	
}


