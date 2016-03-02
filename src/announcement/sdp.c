/*
 * Licence: GPL
 * Created: Fri, 26 Feb 2016 10:53:40 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/sdp/gstsdpmessage.h>
#include <arpa/inet.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* Header file */
#include "../../include/mibParameters.h"
#include "../../include/channelControl/channelTable.h"
#include "../../include/announcement/sdp.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/streaming/stream.h"

/* define constant value for the VIVOE norm */
#define network_type 		"IN"
#define address_type 		"IPV4"
#define address_ttl 		15
#define media_type 			"video"
#define transport_proto 	"RTP/AVP"

/**
 * \brief 	provides a mapping between the int representation of the interlaced-mode of a stream 
 * 			according to the MIB: (1) interlaced, (2) progressive, (3) none
 * \param 	interlaced_mode the int representation of our video stream
 * \return 	the string representation of our interlaced_mode
 */
static char* interlace_mode_to_string(int interlaced_mode){
	switch(interlaced_mode){
		case 1: return "interlaced";
				break;
		case 2: return "progressive";
				break;
		case 3: return "none";
				break;
		default: return "";
	}
}

/**
 * \brief create Media for in RAW video style
 * \param channel_entry an entry in the channelTable to retrieve relevant inforlation
 * \param media the media to modify: where to store the new information
 */
static gboolean create_raw_media(struct channelTable_entry * channel_entry, GstSDPMedia *media)
{
	stream_data *data = channel_entry->stream_datas;
	gchar *fmtp =  g_strdup_printf ("%ld sampling=%s; width=%ld; height=%ld; depth=%ld; colorimetry=%s; %s",
										data->rtp_datas->rtp_type,
										channel_entry->channelVideoSampling,
										channel_entry->channelVertRes,
										channel_entry->channelHorzRes, 
										channel_entry->channelVideoBitDepth,
										channel_entry->channelColorimetry,
										interlace_mode_to_string (channel_entry->channelInterlaced)
										);	
	if( gst_sdp_media_add_attribute (media, "fmtp", fmtp)!= GST_SDP_OK ){
		g_printerr("ERROR: problem in media creation for SDP file\n");
		return FALSE;
	}
	return TRUE;
}

/**
 * \brief create Media for in MPEG-4 video style
 * \param channel_entry an entry in the channelTable to retrieve relevant inforlation
 * \param media the media to modify: where to store the new information
 */
static gboolean create_mpeg4_media(struct channelTable_entry * channel_entry, GstSDPMedia *media)
{
	stream_data *data = channel_entry->stream_datas;
	gchar  		*fmtp =  g_strdup_printf("%ld  profile-level-id=%s, config=%s",
											data->rtp_datas->rtp_type,
											data->rtp_datas->profile_level_id,
											data->rtp_datas->config
										);	
	if( gst_sdp_media_add_attribute (media, "fmtp", fmtp)!= GST_SDP_OK ){
		g_printerr("ERROR: problem in media creation for SDP file\n");
		return FALSE;
	}
	return TRUE;	
}

/**
 * \brief an function that display an appropriate error message
 * \return FALSE (as it is an error ;) )
 */
static gboolean error_function(){
	g_printerr("ERROR:Failed SDP file\n");
		return FALSE;
}

/**
 * \brief create Media for in RAW video style
 * \param channel_entry an entry in the channelTable to retrieve relevant inforlation
 * \param media the media to modify: where to store the new information
 */
static gboolean create_fmtp_media(struct channelTable_entry * channel_entry, GstSDPMedia *media)
{
	if ( !strcmp( channel_entry->channelVideoFormat, "RAW")){ // case RAW format
		if( !create_raw_media(channel_entry, media))
			return error_function();
	}else if ( !strcmp(channel_entry->channelVideoFormat , "MP4V-ES")){ // case MPEG-4 format
		if( !create_mpeg4_media(channel_entry, media))
			return error_function();
	}/*else if ( !strcmp( channel_entry->channelVideoFormat , "J2000")){ // case J2000 format
		if( !create_mpeg4_media(channel_entry, media))
			return error_function();
	}*/else{
		return error_function();
	}
	return TRUE;
}

/** 
 * \brief Create the SDP message corresponding to the stream
 * \pramam entry an entry to the correspondante channel 
 */
gboolean create_SDP(gpointer entry ){

	GstSDPMessage 	*msg;
	struct channelTable_entry * channel_entry = entry; 
	stream_data 	*data = channel_entry->stream_datas;

	/* check channel status, this is check as a stop condition
	 * if the status in stop, we return false, which means that 
	 * we will stop to call repeteadly create_SDP
	 */
	if( channel_entry->channelStatus == stop )
		return FALSE;

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
	int session_version 	= session_id;

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
									network_type, /* network type :internet (always for VIVOE)*/
									address_type, /* address type :IPV4 (always for VIVOE)*/
									inet_ntoa(ip_addr) /* multicast IP*/
								); 

	/* session name
	* s=<session name>
	*/
	/* Build session name from deviceUserDesc Value and DeviceChannel value */
	/* Create a buffer that will contain the concatenation of deviceUserDesc and ChannelUserDesc 
	 * Both of those Strings are 64 bytes strings, so at most, session name will be a 129 bytes string
	 * because we will add a space " " character between those two strings
	 */
	char session_name[DisplayString64*2+1];
	/* Get deviceUserDesc Value */
	memcpy(session_name, deviceInfo.parameters[num_DeviceUD]._value.string_val, strlen(deviceInfo.parameters[num_DeviceUD]._value.string_val));
	strcat(session_name, " ");	
	strcat(session_name, channel_entry->channelUserDesc);
	gst_sdp_message_set_session_name(msg, session_name);

	/* connection:
	 * c=<network type> <address type> <address>
	 */
	gst_sdp_message_set_connection (msg, network_type, address_type,  inet_ntoa(ip_addr) , address_ttl, 0);

	
	/* time : shall be set yo 0 0 in VIVOE to describe an unbound session
	 *
	 * t=<start time> <stop time>
	 */
	gst_sdp_message_add_time(msg, "0", "0", NULL);

	/* media: 
	 * m=<media> <port> <transport> <fmt list>
	 */
	GstSDPMedia *media;
	if( gst_sdp_media_new (&media)!= GST_SDP_OK)
		return error_function();
	if(	gst_sdp_media_set_media (media, media_type )!= GST_SDP_OK)
		return error_function();
	if (gst_sdp_media_set_port_info (media, DEFAULT_MULTICAST_PORT, 1 )!= GST_SDP_OK)
		return error_function();
	if ( gst_sdp_media_set_proto ( media,transport_proto )!= GST_SDP_OK)
		return error_function();

	if( gst_sdp_media_add_format(media, g_strdup_printf ("%ld", data->rtp_datas->rtp_type))!= GST_SDP_OK)
		return error_function();

	/* convert rtp_type into string, max value is 107 so a 4 bytes string */
	gchar *rtmpmap =  g_strdup_printf ("%ld %s/%d", data->rtp_datas->rtp_type,channel_entry->channelVideoFormat ,data->rtp_datas->clock_rate );
	if( gst_sdp_media_add_attribute (media, "rtpmap",rtmpmap ) != GST_SDP_OK)
		return error_function();

	create_fmtp_media(channel_entry, media);

	/* Add framerate to media information */
	gchar *framerate =  g_strdup_printf("%ld", channel_entry->channelFps);
	if( gst_sdp_media_add_attribute (media, "framerate", framerate)!= GST_SDP_OK )	/* Add the media to SDP message */
		return error_function();		
	if ( gst_sdp_message_add_media (msg, media))
		return error_function();		

	printf("%s\n", gst_sdp_message_as_text(msg));
	 return TRUE;	
}
