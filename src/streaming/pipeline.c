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
#include "../../include/conf/mib-conf.h"
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/channelControl/channelTable.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/multicast.h"
#include "../../include/mibParameters.h"
#include "../../include/streaming/filter.h"
#include "../../include/streaming/detect.h"
#include "../../include/streaming/pipeline.h"
#include "../../include/streaming/stream.h"


/*
 * This function add the RTP element to the pipeline
 */
static GstElement* addRTP( 	GstElement *pipeline, 	GstBus *bus,
							guint bus_watch_id, 	GMainLoop *loop,
							GstElement* input, 		struct videoFormatTable_entry *video_info,
							gpointer stream_datas){
	/*Create element that will be add to the pipeline */
	GstElement *rtp = NULL;
	GstElement *parser;
	GstStructure* video_caps;
	
	/* Media stream Type detection */
	video_caps = type_detection(GST_BIN(pipeline), input, loop, NULL);
	/* Fill the MIB a first Time */
	fill_entry(video_caps, video_info, stream_datas);
	
 	/* in case RAW video type has been detected */
	if ( gst_structure_has_name( video_caps, "video/x-raw")){
		/* For Raw video */
		rtp 	= gst_element_factory_make ("rtpvrawpay", "rtp");
		/* Check if everything went ok */
		if( rtp == NULL){
			g_printerr ( "error cannot create element for: %s\n","rtp");
			return NULL;
		}
	}
	/* in case MPEG4 video type has been detected */
	else if  (gst_structure_has_name( video_caps, "video/mpeg")){
		/* For MPEG-4 video */
		parser 	= gst_element_factory_make ("mpeg4videoparse", "parser");
		if( parser == NULL){
			g_printerr ( "error cannot create element for: %s\n","parser");
			return NULL;
		}
		rtp 	= gst_element_factory_make ("rtpmp4vpay", "rtp");
		/* Check if everything went ok */
		if( rtp == NULL){
			g_printerr ( "error cannot create element for: %s\n","rtp");
			return NULL;
		}
		gst_bin_add(GST_BIN(pipeline),parser);
		gst_element_link(input, parser);
		input = parser;
	} 
	/* in case J2K video type has been detected */
	else if  (gst_structure_has_name( video_caps, "image/x-j2c")){
		/* For J2K video */
		rtp 	= gst_element_factory_make ("rtpj2kpay", "rtp");
		/* Check if everything went ok */
		if( rtp == NULL){
			g_printerr ( "error cannot create element for: %s\n","rtp");
			return NULL;
		}
	}
 	/* in case the video type detected is unknown */	
	else
	{
		g_printerr("unknow type of video stream\n");
		return NULL;
	}
	/* add rtp to pipeline */
	gst_bin_add(GST_BIN (pipeline), rtp);
	/* Filters out non VIVOE videos, and link input to RTP if video has a valid format*/
	if (!filter_VIVOE(type_detection(GST_BIN(pipeline), input, loop, NULL),input, rtp)){
		return NULL;
	}
	/* Now that wa have added the RTP payloader to the pipeline, we can get the new caps of the video stream*/
	/* Media stream Type detection */
	video_caps = type_detection(GST_BIN(pipeline), rtp, loop, NULL);
	/*Fill the MIB a second time after creating payload*/
	fill_entry(video_caps, video_info, stream_datas);
	
	/* Finally return*/
	return rtp;
}

void set_udpsink_param( GstElement *udpsink, long channel_entry_index){
	/* compute IP */
	long ip 			= define_vivoe_multicast(deviceInfo.parameters[num_ethernetInterface]._value.array_string_val[0], channel_entry_index);

	/* transform IP from long to char * */
	struct in_addr ip_addr;
	ip_addr.s_addr = ip;

	/*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsink),
                    "host", inet_ntoa(ip_addr),
                    "port", DEFAULT_MULTICAST_PORT,
					"sync", FALSE,
                    NULL);

}

/*
 * This function add the UDP element to the pipeline
 */
static GstElement* addUDP( 	GstElement *pipeline, 	GstBus *bus,
							guint bus_watch_id, 	GMainLoop *loop,
							GstElement *input, 		long channel_entry_index){

	/*Create element that will be add to the pipeline */
	GstElement *udpsink;
		
	/* Create the UDP sink */
    udpsink = gst_element_factory_make ("udpsink", "udpsink");
    if(udpsink == NULL){
       g_printerr ( "error cannot create element for: %s\n","udpsink");
	   return NULL;
    }

	set_udpsink_param(udpsink, channel_entry_index);

	
	/* add udpsink to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), udpsink )){
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
GstElement* create_pipeline_videoChannel( 	gpointer stream_datas,
										 	GMainLoop *loop,
											GstElement* input){
	GstElement 		*last;
	stream_data 	*data 	=  stream_datas;
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
					input, 		  video_stream_info,
					stream_datas);

	if(last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}

	/*
	 * Before returning the starting the stream
	 * Add the entry to the Table, if necessary.
	 */
	long channel_entry_index = 0;
	if( initialize_videoFormat(video_stream_info, stream_datas,  &channel_entry_index)){
		g_printerr("Failed to add entry in the videoFormatTable or in channelEntry\n");
		return NULL;
	}

	/* Add UDP element */
	last = addUDP( 	pipeline, 	  		bus,
					bus_watch_id, 		loop,
					last, 	 			channel_entry_index
				);

	if (last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}
	data->sink = last;
	/* Before returning, free the entry created at the begging*/
	free(video_stream_info);
	return last;

}

/**
 * \brief This function add the UDP element to the pipeline / fort a ServiceUser channel
 */
static GstElement* addUDP_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GMainLoop *loop,
								GstCaps* caps, 			struct channelTable_entry * channel_entry){

	/*Create element that will be add to the pipeline */
	GstElement *udpsrc;
		
	/* Create the UDP sink */
    udpsrc = gst_element_factory_make ("udpsrc", "udpsrc");
    if(udpsrc == NULL){
       g_printerr ( "error cannot create element for: %s\n","udpsrc");
	   return NULL;
    }
	
	/* get the multicast IP */
	struct in_addr multicast_group;
	multicast_group.s_addr = channel_entry->channelReceiveIpAddress;
	/*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsrc),
                	"multicast-group", 	inet_ntoa( multicast_group ),
                    "port", 			DEFAULT_MULTICAST_PORT,
					"caps", 			caps, 
                    NULL);

	/* add rtp to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), udpsrc )){
		g_printerr("Unable to add %s to pipeline", gst_element_get_name(udpsrc));
		return NULL;
	}
	return udpsrc;
}


/**
 * \brief This function add the RTP element to the pipeline
 */
static GstElement* addRTP_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GMainLoop *loop,
								GstElement* input, 		struct videoFormatTable_entry *video_info,
								gpointer stream_datas, 	GstCaps *caps){
	/*Create element that will be add to the pipeline */
	GstElement 		*rtp = NULL;
	GstElement 		*parser, *dec, *last = NULL;
	GstStructure 	*video_caps 	= gst_caps_get_structure(caps, 0);
	char 			*encoding  		= (char*) gst_structure_get_string(video_caps, "encoding-name");
	/* Fill the MIB a first Time */
	fill_entry(video_caps, video_info, stream_datas);
	if ( gst_structure_has_field( video_caps, "encoding-name")){
		/* For Raw video */		
		if ( strcmp( "RAW",encoding) == 0 ){
			rtp 	= gst_element_factory_make ("rtpvrawdepay", "rtp");
			/* Check if everything went ok */
			if( rtp == NULL){
				g_printerr ( "error cannot create element for: %s\n","rtp");
				return NULL;
			}
		gst_bin_add(GST_BIN (pipeline), rtp);
		gst_element_link_many( input, rtp, NULL );
		last = rtp;
		}else if  ( strcmp( "MP4V-ES",encoding) == 0 ){
			/* For MPEG-4 video */
			rtp 	= gst_element_factory_make ("rtpmp4vdepay", "rtp");
			/* Checek if everything went ok */
			if( rtp == NULL){
				g_printerr ( "error cannot create element for: %s\n","rtp");
				return NULL;
			}
			parser = gst_element_factory_make ("mpeg4videoparse", "parser");
			if( parser == NULL){
				g_printerr ( "error cannot create element for: %s\n","parser");
				return NULL;
			}
			dec = gst_element_factory_make ("avdec_mpeg4", "dec");
			/* save last pipeline element */
			if(dec == NULL){
				g_printerr ( "error cannot create element for: %s\n","dec");
				return NULL;
			}
			gst_bin_add_many(GST_BIN (pipeline), rtp, parser, dec, NULL);
			gst_element_link_many( input, rtp, parser, dec, NULL );
			last = dec;
		} 		/* For J2K video */		
		else if ( strcmp( "JPEG2000",encoding) == 0 ){
			rtp 	= gst_element_factory_make ("rtpj2kdepay", "rtp");
			/* Check if everything went ok */
			if( rtp == NULL){
				g_printerr ( "error cannot create element for: %s\n","rtp");
				return NULL;
			}
			dec = gst_element_factory_make ("openjpegdec", "dec");
			/* save last pipeline element */
			if(dec == NULL){
				g_printerr ( "error cannot create element for: %s\n","dec");
				return NULL;
			}
			gst_bin_add_many(GST_BIN (pipeline), rtp, dec, NULL);
			gst_element_link_many( input, rtp, dec, NULL );
			last = dec;
		}
		else {
			g_printerr("unknow type of video stream\n");
			return NULL;
		}
	}else{
		g_printerr("ERROR: encoding format not found\n");
		return NULL;
	}
	
	/* Finally return*/
	return last;
}

#if 0
static gboolean vivoe_redirect_module(gchar *cmdline){

	gchar **splitted;
	/* parse entirely the command line */
	splitted = g_strsplit ( cmdline , " ", -1);
	/* get the encoding parameter */
	if (g_strv_contains ((const gchar * const *) splitted, "encoding"))
	/* free splitted */
}
#endif


/*
 * This function add the UDP element to the pipeline / fort a ServiceUser channel
 */
static GstElement* addSink_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GMainLoop *loop,
								GstElement *input, 		struct channelTable_entry * channel_entry,
								gchar *cmdline
								){

	GError 		*error 		= NULL; /* an Object to save errors when they occurs */
	GstElement 	*sink 		= NULL; /* to return last element of pipeline */

	sink  = gst_parse_bin_from_description (cmdline,
												TRUE,
												&error);

	/* Create the sink */
    if(sink == NULL){
       g_printerr ( "error cannot create element for: %s\n","sink");
	   return NULL;
    }
	/* add rtp to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), sink )){
		g_printerr("Unable to add %s to pipeline", gst_element_get_name(sink));
		return NULL;
	}

	/* we link the elements together */
	if ( !gst_element_link_many (input, sink, NULL)){
		g_print ("Failed to build sink!\n");
	    return NULL;
	}

	return sink;
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
GstElement* create_pipeline_serviceUser( gpointer stream_datas,
									 	 GMainLoop *loop,
										 GstCaps 	*caps,
										 struct channelTable_entry * channel_entry){
	stream_data 	*data 	=  stream_datas;
	/* create the empty videoFormatTable_entry structure to intiate the MIB */
	struct videoFormatTable_entry * video_stream_info;
	video_stream_info = SNMP_MALLOC_TYPEDEF(struct videoFormatTable_entry);
	if( !video_stream_info){
		g_printerr("Failed to create temporary empty entry for the table\n");
		return NULL;
	}

	
	gchar 		*cmdline 	= init_sink_from_conf( channel_entry->channelIndex );

	/* check if everything went ok */	
	if (cmdline == NULL)
		return NULL;

	GstElement 	*pipeline 		= data->pipeline;
	GstBus 		*bus 			= data->bus;
    guint 		bus_watch_id 	= data->bus_watch_id;


	GstElement *first, *last;
	first =  addUDP_SU( pipeline, 		bus,
						bus_watch_id, 	loop,
						caps, 			channel_entry);
	/* one element in pipeline: first is last, and last is first */
	last = first;

	/* Add RTP depayloader element */
	last = addRTP_SU( 	pipeline, 		bus,
						bus_watch_id, 	loop,
						first, 			video_stream_info,
						data, 			caps);

	channelTable_fill_entry(channel_entry, video_stream_info);	

	addSink_SU( pipeline, bus, bus_watch_id, loop, last, channel_entry, cmdline);
	return first;
}
