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
#include <gstreamer-1.0/gst/rtp/gstrtppayloads.h>
#include <arpa/inet.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* Header file */
#include "../../include/mibParameters.h"
#include "../../include/multicast.h"
#include "../../include/deviceInfo/ethernetIfTable.h"
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/channelControl/channelTable.h"
#include "../../include/announcement/sdp.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/streaming/stream.h"
#include "../../include/announcement/gstreamer_function.h"


/**
 * \brief 	provides a mapping between the int representation of the interlaced-mode of a stream
 * 			according to the MIB: (1) interlaced, (2) progressive, (3) none
 * \param 	interlaced_mode the int representation of our video stream
 * \return 	the string representation of our interlaced_mode
 */
static const char* interlace_mode_to_string(int interlaced_mode_val){
	const char * enum_names []=  { NULL, "interlaced" , "progressive" , "none" , NULL };	
	return enum_names[interlaced_mode_val];
}

/**
 * \brief create Media for in RAW video style
 * \param channel_entry an entry in the channelTable to retrieve relevant inforlation
 * \param media the media to modify: where to store the new information
 * \return TRUE on success FALSE otherwise
 */
static gchar *create_raw_media(struct channelTable_entry * channel_entry, GstSDPMedia *media)
{
	stream_data *data = channel_entry->stream_datas;
	gchar *fmtp =  g_strdup_printf ("%ld sampling=%s; width=%ld; height=%ld; depth=%ld; colorimetry=%s; %s",
										data->rtp_datas->rtp_type,
										channel_entry->channelVideoSampling,
										channel_entry->channelHorzRes,
										channel_entry->channelVertRes,
										channel_entry->channelVideoBitDepth,
										channel_entry->channelColorimetry,
										interlace_mode_to_string (channel_entry->channelInterlaced)
									);

	return fmtp;
}

/**
 * \brief create Media for in MPEG-4 video style
 * \param channel_entry an entry in the channelTable to retrieve relevant inforlation
 * \param media the media to modify: where to store the new information
 * \return TRUE on success, FALSE on FAILURE
 */
static gchar *create_mpeg4_media(struct channelTable_entry * channel_entry, GstSDPMedia *media)
{
	stream_data *data = channel_entry->stream_datas;
	gchar  		*fmtp =  g_strdup_printf("%ld  profile-level-id=%s; config=%s",
											data->rtp_datas->rtp_type,
											data->rtp_datas->profile_level_id,
											data->rtp_datas->config
										);
	return fmtp;
}

/**
 * \brief create Media for in J2K video style
 * \param channel_entry an entry in the channelTable to retrieve relevant inforlation
 * \param media the media to modify: where to store the new information
 * \return TRUE on success, FALSE on failure
 */
static gchar *create_j2k_media(struct channelTable_entry * channel_entry, GstSDPMedia *media)
{
	stream_data *data = channel_entry->stream_datas;
	gchar *fmtp =  g_strdup_printf ("%ld sampling=%s; width=%ld; height=%ld;",
										data->rtp_datas->rtp_type,
										channel_entry->channelVideoSampling,
										channel_entry->channelHorzRes,
										channel_entry->channelVertRes/*,
										interlace_mode_to_string (channel_entry->channelInterlaced)*/
									);
	return fmtp;
}


/**
 * \brief an function that display an appropriate error message
 * \return FALSE (as it is an error ;) )
 */
static gboolean error_function(){
	g_printerr("ERROR:Problem SDP file\n");
		return FALSE;
}

/**
 * \brief create Media for in RAW video style
 * \param channel_entry an entry in the channelTable to retrieve relevant inforlation
 * \param media the media to modify: where to store the new information
 * \return TRUE on success, FALSE on failure
 */
static gboolean create_fmtp_media(struct channelTable_entry * channel_entry, GstSDPMedia *media)
{
	gchar *fmtp; /* the string to retrieve the fmtp line of SDP file */
	if ( !strcmp( channel_entry->channelVideoFormat, RAW_NAME )){ // case RAW format
		fmtp = create_raw_media(channel_entry, media) ;
		if ( ! fmtp )
			return error_function();
	}else if ( !strcmp(channel_entry->channelVideoFormat , MPEG4_NAME )){ // case MPEG-6 format
		fmtp = create_mpeg4_media(channel_entry, media) ;
		if ( ! fmtp )
			return error_function();
	}else if ( !strcmp( channel_entry->channelVideoFormat , J2K_NAME)){ // case J2000 format
		fmtp = create_j2k_media(channel_entry, media) ;
		if ( ! fmtp )
			return error_function();
	}else{
		return error_function();
	}

#if 0
	/* if the channel is a ROI, then add roiTop and roiLeft paramerters */
	if ( channel_entry->channelType == roi ){

		/* 
		 * The roi parameters to add to the ftmp SDP's line
		 * Adding a space ' ' at the beginning of this line hase been done on purpose, 
		 * as we will concatenate it to the existing fmtp line
		 */
		gchar *roi_params = g_strdup_printf (" roiTop=%ld; roiLeft=%ld",
										channel_entry->channelRoiOriginTop,
										channel_entry->channelRoiOriginLeft);

		/* extent ftmp buffer size so we can concatenate roi_params string to it */ 
		fmtp = (gchar *) realloc ( fmtp , strlen( fmtp ) + strlen ( roi_params ) + 1 );
		strcat ( fmtp , roi_params ) ;
		free(roi_params);

	}
#endif 

	if( gst_sdp_media_add_attribute (media, "fmtp", fmtp)!= GST_SDP_OK ){
		g_printerr("ERROR: problem in media creation for SDP file\n");
		return error_function();
	}

	free(fmtp); /* memory has been allocated in creta_medianame_media() function */
	return TRUE;
}

/**
 * \brief Create the SDP message corresponding to the stream
 * \param entry an entry to the correspondante channel
 * \return TRUE on success, FALSE on failure
 */
gboolean create_SDP(GstSDPMessage 	*msg, struct channelTable_entry * channel_entry){

	stream_data 	*data = channel_entry->stream_datas;

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
    ip_addr.s_addr			= ethernetIfTable_head->ethernetIfIpAddress;

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
    long ip  = define_vivoe_multicast(deviceInfo.parameters[num_ethernetInterface]._value.array_string_val[0],channel_entry->channelIndex);
	/* transform IP from long to char * */
	struct in_addr multicast_addr;
	multicast_addr.s_addr = ip;
	gst_sdp_message_set_connection (msg, network_type, address_type,  inet_ntoa(multicast_addr) , address_ttl, 0);

	
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
	//printf("%s\n", gst_sdp_message_as_text(msg));
	free(rtmpmap);
	free(framerate);
	 return TRUE;
}


/**
 * \brief Builds a SDP Message structure from a string representation, creates an associated pipeline, starts it
 * \param array: the SDP message obtained from SAP/SDP datagram
 * \param sdp_msg_size: the size of the byte array got from SAP/SDP message
 * \return the caps built from the SDP file extracted from SAP byte array
 */
GstCaps* get_SDP(unsigned char *array, int sdp_msg_size, in_addr_t *multicast_addr, char *channel_desc){

	GstSDPMessage *msg;
	/* create a new SDP message */
	if (gst_sdp_message_new(&msg)){
		g_printerr("Failed to create SDP message\n");
		return NULL;
	}
	
	/* parse the octet string to fill the SDP msg structure */
	if( gst_sdp_message_parse_buffer (array, sdp_msg_size, msg) != GST_SDP_OK){
		g_printerr("Failed to parse SDP message\n");
		return NULL;
	}
	/* get the multicast address of the incoming SDP message */
	const GstSDPConnection *connection;
	connection = gst_sdp_message_get_connection (msg);

	*multicast_addr =  inet_addr(connection->address);
	/* check if the multicast group is indeed, the one we should receive */
	
	const	GstSDPMedia *media;
	if (gst_sdp_message_medias_len (msg) > 1 ){
		g_printerr("ERROR: more than one media in SDP message\n");
		return NULL;
	}
	
	/* get session description */
	strcpy(channel_desc , gst_sdp_message_get_session_name(msg));

	/* get caps from the SDP's media field */
	media 			= gst_sdp_message_get_media(msg , 0);
	GstCaps *caps 	= gst_sdp_media_get_caps_from_media (media, 96);
	
	/* set framerate to caps */
	GValue res = G_VALUE_INIT;
	g_value_init (&res, GST_TYPE_FRACTION);
	int framerate_num = strtol(gst_sdp_media_get_attribute_val (media, "framerate"), NULL, 10);
	gst_value_set_fraction(&res,  framerate_num, 1 );
	
	gst_caps_set_value ( caps,
                  		 "framerate",
                   		&res );

	return caps;
}
