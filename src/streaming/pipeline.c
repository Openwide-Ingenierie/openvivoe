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
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/channelControl/channelTable.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/multicast.h"
#include "../../include/mibParameters.h"
#include "../../include/streaming/filter.h"
#include "../../include/streaming/detect.h"
#include "../../include/streaming/pipeline.h"
#include "../../include/streaming/stream.h"

/**
 * \brief caps of videos for redirection got the name: video/x-rtp, which causes problems to choose the RTP payloader, we want to transform it to video/x-raw,mpeg or image/j2c
 * \param the structure contained in the video caps
 * \return TRUE if we success to transpose the encoding name, FALSE otherwise
 */
static gboolean transpose_encoding_to_name(	GstStructure *video_caps ){

	if ( gst_structure_has_field(video_caps, "encoding-name") ){

		if ( ! strcmp("RAW", g_value_get_string(gst_structure_get_value(video_caps, "encoding-name")) ) )
			gst_structure_set_name(video_caps, "video/x-raw");

		else if ( ! strcmp("JPEG2000", g_value_get_string(gst_structure_get_value(video_caps, "encoding-name")) ) )
			gst_structure_set_name(video_caps, "image/x-j2c");

		else if ( ! strcmp("MP4V-ES", g_value_get_string(gst_structure_get_value(video_caps, "encoding-name")) ) )
			gst_structure_set_name(video_caps, "video/mpeg");

	}else{

		g_printerr("Unkown encoding name, cannot redirect video\n");	
		return FALSE;

	}

	return TRUE;

}

/*
 * This function add the RTP element to the pipeline
 */
static GstElement* addRTP( 	GstElement *pipeline, 	GstBus 							*bus,
							guint bus_watch_id, 	GMainLoop 						*loop,
							GstElement* input, 		struct videoFormatTable_entry 	*video_info,
							gpointer stream_datas, 	GstCaps 						*caps){
	/*Create element that will be add to the pipeline */
	GstElement *rtp = NULL;
	GstElement *capsfilter;
	GstElement *parser;
	GstStructure *video_caps;
	
	if (caps == NULL){
		/* Media stream Type detection */
		video_caps = type_detection(GST_BIN(pipeline), input, loop, NULL);
		/* Fill the MIB a first Time */
		fill_entry(video_caps, video_info, stream_datas);
 	}else{
		video_caps = gst_caps_get_structure ( caps, 0 );
		transpose_encoding_to_name(	video_caps );
		/* This is a redirection, fill the MIB once for all */
		fill_entry(video_caps, video_info, stream_datas);
	}
	
 	/* in case RAW video type has been detected */
	if ( gst_structure_has_name( video_caps, "video/x-raw") ){
		/* For Raw video */
		rtp 	= gst_element_factory_make ("rtpvrawpay", "rtpvrawpay");
		/* Check if everything went ok */
		if( rtp == NULL){
			g_printerr ( "error cannot create element for: %s\n","rtpvrawpay");
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
		rtp 	= gst_element_factory_make ("rtpmp4vpay", "rtpmp4vpay");
		/* Check if everything went ok */
		if( rtp == NULL){
			g_printerr ( "error cannot create element for: %s\n","rtpmp4vpay");
			return NULL;
		}
		gst_bin_add(GST_BIN(pipeline),parser);
		gst_element_link(input, parser);
		input = parser;
	}

	/* in case J2K video type has been detected */
	else if  (gst_structure_has_name( video_caps, "image/x-j2c")){
		/* For J2K video */
		rtp 	= gst_element_factory_make ("rtpj2kpay", "rtpj2kpay");
		/* Check if everything went ok */
		if( rtp == NULL){
			g_printerr ( "error cannot create element for: %s\n","rtpj2kpay");
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

	if (caps == NULL ){
		/* Filters out non VIVOE videos, and link input to RTP if video has a valid format*/
		if (!filter_VIVOE(type_detection(GST_BIN(pipeline), input, loop, NULL),input, rtp)){
			return NULL;
		}

		/* Now that wa have added the RTP payloader to the pipeline, we can get the new caps of the video stream*/
		/* Media stream Type detection */
		video_caps = type_detection(GST_BIN(pipeline), rtp, loop, NULL);
		/*Fill the MIB a second time after creating payload*/
		fill_entry(video_caps, video_info, stream_datas);
	}else{
	
		capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
		g_object_set(capsfilter, "caps",gst_caps_new_full (gst_structure_new( 	"video/x-raw" 	,
															"format" 		, G_TYPE_STRING ,"I420" ,
															"width" 		, G_TYPE_INT 	, 1920,
															"height" 		, G_TYPE_INT 	, 1080,
															"interlace-mode", G_TYPE_STRING , "progressive",
															"framerate" 	, GST_TYPE_FRACTION, 30, 1,
															NULL) , NULL)  , NULL);

		gst_bin_add(GST_BIN(pipeline),capsfilter);

		/* link input to rtp payloader */
		if ( !gst_element_link_many(input, capsfilter, rtp, NULL)){
			g_printerr("ERROR: failed link rtp payloader\n");
		   return NULL;	
		}
		
		/*Fill the MIB a second time after creating payload, this is needed to get rtp_data needed to build SDP files */
		fill_entry(video_caps, video_info, stream_datas);

	}

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
											GstElement* input,
											long videoChannelIndex){
	GstElement 		*last;
	stream_data 	*data 	=  stream_datas;
	/* create the empty videoFormatTable_entry structure to intiate the MIB */
	struct videoFormatTable_entry * video_stream_info;
	video_stream_info = SNMP_MALLOC_TYPEDEF(struct videoFormatTable_entry);
	video_stream_info->videoFormatIndex = videoChannelIndex;
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
					stream_datas, NULL);
	
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
		g_printerr("Failed to add entry in the videoFormatTable or in channelTable\n");
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
 * \brief add elements to the redirection source's pipeline, now that we know the rtp payloader to add
 * \param caps tha video caps of the stream received by the serviceUser to redirect
 * \param channel_entry the service User that receives the stream we need to redirect
 */
GstElement *append_SP_pipeline_for_redirection( GstCaps *caps, long videoFormatIndex ){

	GstElement 		*last;
	
	
	/* get the entry in videoFormatTable that corresponds to the mapping */
	struct videoFormatTable_entry* videoFormat_entry = videoFormatTable_getEntry(videoFormatIndex);
	
	stream_data 	*data 	=  videoFormat_entry->stream_datas;

	if ( data == NULL ){
		g_printerr("Failed to append pipeline for redirection\n");
		return NULL;
	}


	/* Now build the corresponding pipeline according to the caps*/ 

	last = addRTP(data->pipeline , data->bus , data->bus_watch_id , loop , data->sink  , videoFormat_entry , data , caps);
	if(last == NULL){
		g_printerr("Failed to append pipeline for redirection\n");
		return NULL;
	}

	/* get the index of the channel corresponding to the videoFormatIndex */
	struct channelTable_entry *channel_entry = channelTable_get_from_VF_index(videoFormatIndex);

	/* fill the channel Table */	
	channelTable_fill_entry ( channel_entry , videoFormat_entry );

	/* Add UDP element */
	last = addUDP(  data->pipeline, 	data->bus,
					data->bus_watch_id, loop,
					last, 	 			channel_entry->channelIndex
				);

	if (last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}

	/* foe convenience sinl contained the source element "intervideosrc" but it is not it purposes */
	data->sink = last;
	channel_entry->stream_datas = data;
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
 * \param pipeline the associated pipeline of the channel
 * \param bus the bus the channel
 * \param bus_watch_id an id watch on the bus
 * \param loop the GMainLoop
 * \param input the gstelement to link in input
 * \param video_info a videoFormatTable_entry to store detected caps 
 * \param stream_data the stream_data associated to the pipeline
 * \param caps the input video caps
 * \return GstElement the last element added in pipeline 
 */
static GstElement* addRTP_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GMainLoop *loop,
								GstElement* input, 		struct videoFormatTable_entry *video_info,
								gpointer stream_datas, 	GstCaps *caps){

	/*Create element that will be add to the pipeline */
	GstElement 		*rtp = NULL;
	GstElement 		*last = NULL;
	GstStructure 	*video_caps 	= gst_caps_get_structure(caps, 0);
	char 			*encoding  		= (char*) gst_structure_get_string(video_caps, "encoding-name");
	/* Fill the MIB a first Time */
	fill_entry(video_caps, video_info, stream_datas);
	if ( gst_structure_has_field( video_caps, "encoding-name")){
		/* For Raw video */		
		if ( strcmp( "RAW",encoding) == 0 ){
			rtp 	= gst_element_factory_make ("rtpvrawdepay", "rtpdepay");
			/* Check if everything went ok */
			if( rtp == NULL){
				g_printerr ( "error cannot create element for: %s\n","rtpdepay");
				return NULL;
			}
		}else if  ( strcmp( "MP4V-ES",encoding) == 0 ){
			/* For MPEG-4 video */
			rtp 	= gst_element_factory_make ("rtpmp4vdepay", "rtpdepay");
			/* Checek if everything went ok */
			if( rtp == NULL){
				g_printerr ( "error cannot create element for: %s\n","rtpdepay");
				return NULL;
			}
		} 		/* For J2K video */		
		else if ( strcmp( "JPEG2000",encoding) == 0 ){
			rtp 	= gst_element_factory_make ("rtpj2kdepay", "rtpdepay");
			/* Check if everything went ok */
			if( rtp == NULL){
				g_printerr ( "error cannot create element for: %s\n","rtpdepay");
				return NULL;
			}
		}
		else {
			g_printerr("unknow type of video stream\n");
			return NULL;
		}
	}else{
		g_printerr("ERROR: encoding format not found\n");
		return NULL;
	}

	gst_bin_add(GST_BIN (pipeline), rtp);
	gst_element_link_many( input, rtp, NULL );
	last = rtp;

	/* Finally return*/
	return last;

}

/*
 * This function add the UDP element to the pipeline / fort a ServiceUser channel
 */
static GstElement* addSink_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GMainLoop *loop,
								GstElement *input, 		struct channelTable_entry * channel_entry,
								gchar *cmdline, 		gboolean 					redirect
								){

	GError 		*error 		= NULL; /* an Object to save errors when they occurs */
	GstElement 	*sink 		= NULL; /* to return last element of pipeline */
	
	if ( !redirect){
		sink  = gst_parse_bin_from_description (cmdline,
												TRUE,
												&error);
	}else{
		sink = gst_element_factory_make("shmsink"/*"intervideosink"*/, "redirect-sink");
		g_object_set(sink, "socket-path" , "/tmp/testvivoe" ,"shm-size" , 100000000 , "wait-for-connection" , 1, "sync", FALSE , NULL);
	}

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
GstElement* create_pipeline_serviceUser( gpointer 					stream_datas,
									 	 GMainLoop 					*loop,
										 GstCaps 					*caps,
										 struct channelTable_entry 	*channel_entry,
										 gchar 						*cmdline,
										 gboolean 					redirect){

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


	GstElement *first, *last;

	first =  addUDP_SU( pipeline, 		bus,
						bus_watch_id, 	loop,
						caps, 			channel_entry);

	/* check if everything went ok */
	if ( first == NULL )
		return NULL;

	/* one element in pipeline: first is last, and last is first */
	last = first;

	/* Add RTP depayloader element */
	last = addRTP_SU( 	pipeline, 		bus,
						bus_watch_id, 	loop,
						first, 			video_stream_info,
						data, 			caps);

	channelTable_fill_entry(channel_entry, video_stream_info);

	last = addSink_SU( pipeline, bus, bus_watch_id, loop, last, channel_entry, cmdline, redirect);

	return last;
}
