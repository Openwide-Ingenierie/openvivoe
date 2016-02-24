/*
 * Licence: GPL
 * Created: Mon, 08 Feb 2016 09:32:51 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gst/video/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <arpa/inet.h>
#include "../../include/mibParameters.h"
#include "../../include/streaming/filter.h"
#include "../../include/streaming/detect.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/streaming/pipeline.h"
#include "../../include/streaming/stream.h"

/*
 * This function add the RTP element to the pipeline
 */
static GstElement* addRTP( 	GstElement*pipeline, 	GstBus *bus,
							guint bus_watch_id, 	GMainLoop *loop,
							GstElement* input, 		struct videoFormatTable_entry *video_info){
	/*Create element that will be add to the pipeline */
	GstElement *rtp = NULL;
	GstStructure* video_caps;
	/* Media stream Type detection */
	video_caps = type_detection(GST_BIN(pipeline), input, loop);
	/* Fill the MIB a first Time */
	fill_entry(video_caps, video_info);

	if ( gst_structure_has_name( video_caps, "video/x-raw")){
		/* For Raw video */
		rtp 	= gst_element_factory_make ("rtpvrawpay", "rtp");
		/* Check if everything went ok */
		if( rtp == NULL){
			g_printerr ( "error cannot create element for: %s\n","rtp");
			return NULL;        
		}
	}else if  (gst_structure_has_name( video_caps, "video/mpeg")){
		/* For MPEG-4 video */
		rtp 	= gst_element_factory_make ("rtpmp4vpay", "rtp");
		/* Check if everything went ok */
		if( rtp == NULL){
			g_printerr ( "error cannot create element for: %s\n","rtp");
			return NULL;        
		}
	}else{
		g_printerr("unknow type of video stream\n");
		return NULL;
	}
	/* add rtp to pipeline */
	gst_bin_add(GST_BIN (pipeline), rtp);
	/* Filters out non VIVOE videos, and link input to RTP if video has a valid format*/ 
	if (!filter_VIVOE(type_detection(GST_BIN(pipeline), input, loop),input, rtp)){
		return NULL;
	}
	/* Now that wa have added the RTP payloader to the pipeline, we can get the new caps of the video stream*/
	/* Media stream Type detection */
	video_caps = type_detection(GST_BIN(pipeline), rtp, loop);
	/*Fill the MIB a second time after creating payload*/	
	fill_entry(video_caps, video_info);	

	/* Finally return*/	
	return rtp;
}

/*
 * This function add the UDP element to the pipeline
 */
static GstElement* addUDP( 	GstElement*pipeline, 	GstBus *bus,
							guint bus_watch_id, 	GMainLoop *loop,
							GstElement* input, 		long ip){
	/*Create element that will be add to the pipeline */
	GstElement *udpsink;
		
	/* Create the UDP sink */
    udpsink = gst_element_factory_make ("udpsink", "udpsink");
    if(udpsink == NULL){
       g_printerr ( "error cannot create element for: %s\n","udpsink");
	   return NULL;
    }

	/* transform IP from long to char * */
	struct in_addr ip_addr;
	ip_addr.s_addr = ip;
	/*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsink),
                    "host", inet_ntoa(ip_addr),
                    "port", DEFAULT_MULTICAST_PORT,
                    NULL);

	/* add rtp to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline),  udpsink )){
		g_printerr("Unable to add %s to pipeline", gst_element_get_name(udpsink));
		return NULL;
	}
	/* we link the elements together */
	if ( !gst_element_link_many (input, udpsink, NULL)){
		g_print ("Failed to build UDP sink!\n");
	    return NULL;
	}
	return udpsink;
}

/**
 * \brief Create the pipeline, add information to MIB at the same time
 * \param pipeline the pipepline of the stream
 * \param bus the bus associated to the pipeline
 * \param bust_watch_id the watch associated to the bus
 * \param loop the GMainLoop
 * \param input the las element of the pipeline, (avenc_mp4 or capsfilter) as we built our own source 
 * \param ip the ip to which send the stream on
 * \port the port to use
 * \return GstElement* the last element added to the pipeline (should be udpsink if everything went ok)
 */
GstElement* create_pipeline( gpointer stream_datas,
						 	 GMainLoop *loop,
							 GstElement* input){
	GstElement* last;
	stream_data *data 	=  stream_datas;
	long 				ip;
	/* create the empty videoFormatTable_entry structure to intiate the MIB */	
	struct videoFormatTable_entry * video_stream_info;
	video_stream_info = SNMP_MALLOC_TYPEDEF(struct videoFormatTable_entry);
	if( !video_stream_info){
		g_printerr("Failed to create temporary empty entry for the table\n");
		return NULL;
	}

	GstElement 	*pipeline 		= data->pipeline;
	GstBus 		*bus 			= data->bus;
    guint 		bus_watch_id 	= data->bus_watch_id;
   	/* Add RTP element */
 	last = addRTP( 	pipeline, 	  bus,
					bus_watch_id, loop,
					input, 		  video_stream_info);

	if(last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}

	/* 
	 * Before returning the starting the stream
	 * Add the entry to the Table, if necessary.
	 */
	if( initialize_videoFormat(video_stream_info, stream_datas, &ip)){
		g_printerr("Failed to add entry in the videoFormatTable or in channelEntry\n");
		return NULL;
	}	

	/* Add UDP element */
	last = addUDP( 	pipeline, 	  bus,
					bus_watch_id, loop,
					last, ip);

	if (last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}	

	/* Before returning, free the entry created at the begging*/
	free(video_stream_info);	
	return last;

}
