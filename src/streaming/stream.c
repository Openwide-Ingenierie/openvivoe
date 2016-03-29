/*
 * Licence: GPL
 * Created: Tue, 09 Feb 2016 14:58:20 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#include <signal.h>
#include <glib-2.0/glib.h>
#include <glib-unix.h>
#include <gstreamer-1.0/gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
/* In order to initialize the MIB */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../../include/mibParameters.h"
#include "../../include/conf/mib-conf.h"
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/channelControl/channelTable.h"
#include "../../include/streaming/pipeline.h"
#include "../../include/streaming/detect.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/streaming/stream.h"
/*
 * Macro for testing purposes
 */
#define DEFAULT_FORMAT "raw" /*this is the default video format used for the stream */

/**
 * \brief Handle the Bus: display message, get interruption, etc...
 * \param bus the Bus element
 * \param msg the masseges receive on the bus
 * \return gboolean TRUE (always)
 */
static gboolean bus_call (  GstBus     *bus,
							GstMessage *msg,
							gpointer    data)
{
	GMainLoop *loop = (GMainLoop *) data;
	switch (GST_MESSAGE_TYPE (msg)) {

		case GST_MESSAGE_EOS:
			g_print ("End of stream\n");
			g_main_loop_quit (loop);
			break;

		case GST_MESSAGE_ERROR:{
								   gchar  *debug;
								   GError *error;
								   gst_message_parse_error (msg, &error, &debug);
								   g_free (debug);

								   g_printerr ("Error: %s\n", error->message);
								   g_error_free (error);
								   g_main_loop_quit (loop);
							   }
			break;

		case GST_MESSAGE_WARNING:{
								   gchar  *debug;
								   GError *error;
								   gst_message_parse_warning (msg, &error, &debug);
								   g_free (debug);

								   g_printerr ("Error: %s\n", error->message);
								   g_error_free (error);
								   g_main_loop_quit (loop);
							   }
			break;

		default:
			break;
	}

	return TRUE;
}
#if 0
/**
 * \brief create a fake source for test purposes
 * \param pipeline: the pipeline in which add the source
 * \param format: the video format for the source raw or mp4
 * \param width the width to give to the video source
 * \param height the height to give to the video source
 * \param encoding the encoding to use for the video source
 * \return GstElement* the last element add and link into the pipeline
 */
static GstElement* source_creation(GstElement* pipeline, char* format, int width, int height/*, char* encoding*/){
	/*
	 * For now, the source is created manually, directly into the code
	 * */
	GstElement 	*source, *capsfilter, *enc, *last;
	GstCaps 	*caps;
	/*
	 * For now, the source is created manually, directly into the code
	 * */
	source = gst_element_factory_make ("v4l2src", "source");
//	source = gst_element_factory_make ("videotestsrc", "source");
	if(source == NULL){
		g_printerr ("error cannot create element: %s\n", "videotestsrc" );
		return NULL;
	}
	g_object_set(source, "device", "/dev/video0", NULL);
	capsfilter = gst_element_factory_make ("capsfilter","capsfilter");
	if(capsfilter == NULL){
		g_printerr ( "error cannot create element: %s\n", "capsfilter" );
		return NULL;
	}

	caps = gst_caps_new_full( 	gst_structure_new( 	"video/x-raw" 	,
													"format" 		, G_TYPE_STRING ,"I420" ,
													"width" 		, G_TYPE_INT 	, width,
													"height" 		, G_TYPE_INT 	, height,
													"interlace-mode", G_TYPE_STRING , "progressive",
													"framerate" 	, GST_TYPE_FRACTION, 30, 1,
													NULL),
								NULL);
	g_return_if_fail (gst_caps_is_fixed (caps));

	/* Put the source in the pipeline */
	g_object_set (capsfilter, "caps", caps, NULL);
	gst_bin_add_many (GST_BIN(pipeline), source, capsfilter, NULL);
	gst_element_link (source, capsfilter);
	/* save last pipeline element*/
	last = capsfilter;
	/* For test purposes, if user specify mp4 as format, encrypt the vidoe */
	if( !strcmp(format, "MP4V-ES")){
		/* For test puposes, if we wanna test our program with a fake mpeg-4 source, it necessary to create is mannually with an encoder */
		/*create the MPEG-4 encoder */
		enc = gst_element_factory_make ("avenc_mpeg4", "enc");
		/* save last pipeline element */
		if(enc == NULL){
			g_printerr ( "error cannot create element for: %s\n","enc");
			return NULL;
		}
		/* add encryptor to pipeline, link it to the source */
		gst_bin_add_many (GST_BIN(pipeline), enc, NULL);
		gst_element_link_many (capsfilter, enc,NULL);
		last = enc;
	}
	if( !strcmp(format, "JPEG2000")){
		/* For test puposes, if we wanna test our program with a fake mpeg-4 source, it necessary to create is mannually with an encoder */
		/*create the MPEG-4 encoder */
		enc = gst_element_factory_make ("openjpegenc", "enc");
		/* save last pipeline element */
		if(enc == NULL){
			g_printerr ( "error cannot create element for: %s\n","enc");
			return NULL;
		}
		/* add encryptor to pipeline, link it to the source */
		gst_bin_add_many (GST_BIN(pipeline), enc, NULL);
		gst_element_link_many (capsfilter, enc,NULL);
		last = enc;
	}

	return last;
}
#endif

/**
 * \brief specify if a SP is a redirection (check if it is in the redirection_channel array
 * \param videoFormatIndex the index of the videoFormat to check 
 * \return TRUE if it is a redirection, FALSE otherwise
 */
static gboolean SP_is_redirection(long videoFormatIndex){

	int i = 0;
	while ( redirect_channels[i] != NULL ){
		if ( redirect_channels[i]->video_SP_index == videoFormatIndex ) /* if found, then returns */
			return TRUE;
		i++;
	}

	return FALSE; /* if not found, returns FALSE */

}

/** 
 * \brief create emtpy entries in VFT and CT for the redirection entries for a SP
 * \param steam_data the stream data to associate to the entry 
 * \param videoFormatnIndex the index of the VF to create
 */ 
static void init_redirection( gpointer stream_datas, long videoFormatIndex ){
	
	stream_data *data 			= stream_datas;

	/* create an empty entry for the redirection */
	videoFormatTable_createEntry( 	videoFormatIndex, 	videoChannel,
									disable, 			"",
									"", 				0,
									0,				 	"",
									0, 					0,
									0, 					0,
									0, 					0,
									0, 					0,
									0,					0,
									0, 					0,
									data);

	channelTable_create_empty_entry(channelNumber._value.int_val+1, videoFormatIndex, videoChannel, 0, data);

}

/**
 * \brief get the command line to use to get the source from configuration file
 * \param pipeline the pipeline to wich adding the bin made from the cmd line description 
 * return GstElement* the last element added in the pipeline: the bin made
 */
static GstElement *get_source( GstElement* pipeline, long videoFormatIndex){
	GError 		*error 		= NULL; /* an Object to save errors when they occurs */
	GstElement 	*bin 		= NULL; /* to return last element of pipeline */
	gchar 		*cmdline 	= init_sources_from_conf( videoFormatIndex );

	/* check if everything went ok */	
	if (cmdline == NULL)
		return NULL;

	/* check if it is a redirection */
 
	if( SP_is_redirection( videoFormatIndex ) ){
		/* if so build the appropriate source */
		bin = gst_element_factory_make( "intervideosrc", "src-redirection");
		if ( !bin ){
			g_printerr("Failed to parse: %s\n",error->message);
	   		return NULL;
		}
	}
	else
		bin  = gst_parse_bin_from_description ( cmdline,
												TRUE,
												&error);	

	if ( error != NULL){
		g_printerr("Failed to parse: %s\n",error->message);
	   	return NULL;	
	}

	/* add bin in pipeline */
	gst_bin_add (GST_BIN(pipeline), bin);
	free(cmdline);
	return bin;
}

/**
 * \brief intialize the stream: create the pipeline and filters out non vivoe format
 * \param loop the main loop
 * \param stream_datas a structure in which we will save the pipeline and the bus elements
 * \param videoFormatIndex the videoFormatIndex for which the pipeline will be built
 * \return EXIT_SUCCESS or EXIT_FAILURE
 */
static int init_stream_SP( gpointer main_loop, gpointer stream_datas, long videoFormatIndex)
{
	stream_data *data 			= stream_datas;
	GstElement 	*pipeline 		= data->pipeline;
    GstElement 	*last;	

    return EXIT_SUCCESS;
}


/**
 * \brief initiate a pipeline for a SP
 * \param main_loop the main loop
 * \param videoFormatIndex the videoFormatIndex for which the pipeline will be built
 * \return EXIT_SUCCESS or EXIT_FAILURE
 */
int stream_SP( gpointer main_loop, int videoFormatIndex){

	/* Initialization of elements needed */
	GMainLoop 	*loop 			= main_loop;
    GstElement 	*pipeline;
    GstElement 	*last;	
    GstBus 		*bus;
    guint 		bus_watch_id;
	
	/* Create the pipeline */
	pipeline  = gst_pipeline_new ("pipeline");
	if(pipeline  == NULL){
		g_printerr ( "error cannot create: %s\n","pipeline" );
		return EXIT_FAILURE;
	}

	/* Reference all data relevant to the stream */
	stream_data 	*data 		= malloc(sizeof(stream_data));

	/* allocate memory for the structure rtp_data of stream_data */
	/* this will be free in the delete_stream function */
	rtp_data 	*rtp_datas 		= malloc(sizeof(rtp_data));
	data->rtp_datas 			= rtp_datas;
	
	data->pipeline 		= pipeline;	
	/* we add a message handler */
	bus = gst_pipeline_get_bus ( GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

	data->bus 			= bus;
	data->bus_watch_id 	= bus_watch_id;

#if 0
	/* build pipeline */
	if (init_stream_SP( main_loop, data, videoFormatIndex))
		return EXIT_FAILURE;
#endif  //if 0
	last = get_source(pipeline, videoFormatIndex);
	if (last == NULL ){
		g_printerr ( "Failed to create videosource\n");
		return EXIT_FAILURE;
	}
	
	gboolean redirection = FALSE; 
	/* if this is a redirection */
	if ( !strcmp(GST_ELEMENT_NAME(last), "src-redirection") ){ 
		init_redirection( data/*stream_datas*/, videoFormatIndex );
		redirection = TRUE;
	}
	else
		last = create_pipeline_videoChannel( data /* stream_datas*/, 	main_loop, last, videoFormatIndex ); /* Create pipeline  - save videoFormatIndex into stream_data data*/

	/* Check if everything went ok*/
	if (last == NULL){
		g_printerr ( "Failed to create pipeline\n");
		return EXIT_FAILURE;
	}
	/* set pipeline to PAUSED state will open /dev/video0, so it will not be done in start_streaming */
	gst_element_set_state (data->pipeline, GST_STATE_PAUSED);

	if ( ! redirection ){	
		/* if we are in the defaultStartUp Mode, launch the last VF register in the table */
		if ( deviceInfo.parameters[num_DeviceMode]._value.int_val == defaultStartUp){
			struct channelTable_entry *entry 	= channelTable_get_from_VF_index(videoFormatIndex);
			if (entry == NULL ){
				g_printerr("ERROR: try to start a source that has no channel associated\n");
				return EXIT_FAILURE;
			}
			entry->channelStatus 				= start;
			if ( channelSatus_requests_handler( entry ))
				return EXIT_SUCCESS;

			else
				return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;

}

/**
 * \brief initiate a pipeline to display received stream
 * \param main_loop the main loop
 * \param stream_datas a structure in which we will save the pipeline and the bus elements
 * \param caps the GstCaps made from the SDP file received with the stream
 * \param channel_entry the serviceUser corresponding channel 
 * \return EXIT_SUCCESS or EXIT_FAILURE
 */
int init_stream_SU( gpointer main_loop,GstCaps *caps, struct channelTable_entry *channel_entry)
{
    
  /* Initialization of elements needed */
	GMainLoop 	*loop 			= main_loop;
    GstElement 	*pipeline;
    GstBus 		*bus;
    guint 		bus_watch_id;
	GstElement *last;

	/* Create the pipeline */
	pipeline  = gst_pipeline_new ("pipeline");
	if(pipeline  == NULL){
		g_printerr ( "error cannot create: %s\n","pipeline" );
		return EXIT_FAILURE;
	}

	/* Reference all data relevant to the stream */
	stream_data 	*data 		= malloc(sizeof(stream_data));

	/* allocate memory for the structure rtp_data of stream_data */
	/* this will be free in the delete_stream function */
	rtp_data 	*rtp_datas 		= malloc(sizeof(rtp_data));
	data->rtp_datas 			= rtp_datas;
	
	data->pipeline 		= pipeline;	
	/* we add a message handler */
	bus = gst_pipeline_get_bus ( GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

	data->bus 			= bus;
	data->bus_watch_id 	= bus_watch_id;

	gchar 		*cmdline 	= init_sink_from_conf( channel_entry->channelIndex );

	/* check if everything went ok */	
	if (cmdline == NULL )
		return EXIT_FAILURE;

	/* declare the boolean used to check if this is a redirection */
	gboolean redirect = FALSE;

	redirect = vivoe_redirect(cmdline); 

	/* Create pipeline  - save videoFormatIndex into stream_data data*/
	last = create_pipeline_serviceUser( data, main_loop, caps, channel_entry, cmdline, redirect );

	/* Check if everything went ok*/
	if (last == NULL){
		g_printerr ( "Failed to create pipeline\n");
		return EXIT_FAILURE;
	}

	gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
	channel_entry->stream_datas = data;
    return EXIT_SUCCESS;
}


#if 0
/**
 * \brief initiate a pipeline for a SP or a SU
 * \param main_loop the main loop
 * \param caps the GstCaps made from the SDP file received with the stream
 * \param channel_entry the serviceUser corresponding channel 
 * \return EXIT_SUCCESS or EXIT_FAILURE
 */
int init_streaming (gpointer main_loop, GstCaps *caps, struct channelTable_entry * channel_entry)
{
    /* Initialization of elements needed */
	GMainLoop 	*loop 			= main_loop;
    GstElement 	*pipeline;
    GstBus 		*bus;
    guint 		bus_watch_id;
	
	/* Create the pipeline */
	pipeline  = gst_pipeline_new ("pipeline");
	if(pipeline  == NULL){
		g_printerr ( "error cannot create: %s\n","pipeline" );
		return EXIT_FAILURE;
	}

	/* Reference all data relevant to the stream */
	stream_data 	*data 		= malloc(sizeof(stream_data));

	/* allocate memory for the structure rtp_data of stream_data */
	/* this will be free in the delete_stream function */
	rtp_data 	*rtp_datas 		= malloc(sizeof(rtp_data));
	data->rtp_datas 			= rtp_datas;
	
	data->pipeline 		= pipeline;	
	/* we add a message handler */
	bus = gst_pipeline_get_bus ( GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

	data->bus 			= bus;
	data->bus_watch_id 	= bus_watch_id;

	if (init_stream_SU( main_loop, data, caps, channel_entry))
		return EXIT_FAILURE;
	channel_entry->stream_datas = data;	

	return EXIT_SUCCESS;
}
#endif 
/**
 * \brief start the streaming: set pipeling into PLAYING state
 * \param data of the stream (pipeline, bus and bust_watch_id) - see stream_data structure
 * \return 0
 */
gboolean start_streaming (gpointer stream_datas, long channelVideoFormatIndex ){
	stream_data *data 								= stream_datas;
	struct videoFormatTable_entry * stream_entry 	= videoFormatTable_getEntry(channelVideoFormatIndex);
	if ( data->pipeline != NULL){
		/* Set the pipeline to "playing" state*/
		g_print ("Now playing channel: %ld\n",channelVideoFormatIndex );
		gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
		if ( stream_entry != NULL)
			stream_entry->videoFormatStatus = enable;
		return TRUE;
	}
	return FALSE;
}

/**
 * \brief stop the streaming: set pipeling into NULL state
 * \param data of the stream (pipeline, bus and bust_watch_id) - see stream_data structure
 * \return 0
 */
int stop_streaming( gpointer stream_datas, long channelVideoFormatIndex ){
	stream_data *data 								= stream_datas;
	struct videoFormatTable_entry * stream_entry 	= videoFormatTable_getEntry(channelVideoFormatIndex);
	/* Out of the main loop, clean up nicely */
	g_print ("Returned, stopping playback\n");
	gst_element_set_state (data->pipeline, GST_STATE_NULL);
	if ( stream_entry != NULL)	
		stream_entry->videoFormatStatus = disable;
	return 0;
}

/**
 * \brief delete the stream: free pipeline and bus element
 * \param data of the stream (pipeline, bus and bust_watch_id) - see stream_data structure
 * \return 0
 */
int delete_steaming_data(gpointer channel_entry){
	struct channelTable_entry 	*entry 	= channel_entry;
	stream_data 				*data 	= entry->stream_datas;
	/* delete pipeline */
	if (data != NULL ){
		g_print ("Deleting pipeline\n");
		gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
		gst_element_set_state (data->pipeline, GST_STATE_READY);
		gst_element_set_state (data->pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (data->pipeline));
		g_source_remove (data->bus_watch_id);
		/* free rtp_data */
		free(data->rtp_datas);
		/* free the sap_data */
		free(data);
		entry->stream_datas = NULL;
	}
	return 0;
}
