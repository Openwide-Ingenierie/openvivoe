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
#include "../../include/videoFormatInfo/videoFormatTable.h"
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
								   break;
							   }
		default:
			break;
	}

	return TRUE;
}
/**
 * \brief create a fake source for test purposes
 * \param pipeline: the pipeline in which add the source 
 * \param format: the video format for the source raw or mp4
 * \param width the width to give to the video source 
 * \param height the height to give to the video source
 * \param encoding the encoding to use for the video source
 * \return GstElement* the last element add and link into the pipeline 
 */
static GstElement* source_creation(GstElement* pipeline, char* format, int width, int height, char* encoding){
	/*
	 * For now, the source is created manually, directly into the code
	 * */
	GstElement 	*source, *capsfilter, *enc, *last;	
	GstCaps 	*caps;
	/*
	 * For now, the source is created manually, directly into the code
	 * */
	source = gst_element_factory_make ("videotestsrc", "source");
	if(source == NULL){
		g_printerr ("error cannot create element: %s\n", "videotestsrc" );
		return NULL;        
	}

	capsfilter = gst_element_factory_make ("capsfilter","capsfilter");
	if(capsfilter == NULL){
		g_printerr ( "error cannot create element: %s\n", "capsfilter" );
		return NULL;
	}

	caps = gst_caps_new_full( 	gst_structure_new( 	"video/x-raw" 	, 
													"format" 		, G_TYPE_STRING , encoding,
													"width" 		, G_TYPE_INT 	, width,
													"height" 		, G_TYPE_INT 	, height,
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
	if( !strcmp(format, "mp4")){
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
	return last;
}

/**
 * \brief intialize the stream: create the pipeline and filters out non vivoe format
 * \param argc to know which source to built
 * \param argv to know which source to built
 * \param stream_datas a structure in which we will save the pipeline and the bus elements 
 * \return 0 
 */
int init_streaming (gpointer main_loop, gpointer stream_datas /* real prototype */
					, char* format, int width, int height, char* encoding /* extra parameters for testing purposes*/)
{
    /* Initialization of elements needed */
    GstElement 	*pipeline, *last;
    GstBus 		*bus;
    guint 		bus_watch_id;
	GMainLoop 	*loop 				= main_loop;
	
    /* Create the pipeline */
    pipeline  = gst_pipeline_new ("pipeline");
	if(pipeline  == NULL){
		g_printerr ( "error cannot create: %s\n","pipeline" );
		return EXIT_FAILURE;
	}
	
	/* we add a message handler */
	bus = gst_pipeline_get_bus ( GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

	/* Reference all data relevant to the stream */
	stream_data *data 	= stream_datas;
	data->pipeline 		= pipeline;
	data->bus 			= bus;
	data->bus_watch_id 	= bus_watch_id;
	/* Source Creation */
	last = source_creation(pipeline, format,width ,height ,encoding);

	if (last == NULL ){
		g_printerr ( "Failed to create videosource\n");	
		return EXIT_FAILURE;	
	}
	/* Create pipeline  - save videoFormatIndex into stream_data data*/
	last = create_pipeline( stream_datas, 	loop, last );
	/* Check if everything went ok*/ 
	if (last == NULL){
		g_printerr ( "Failed to create pipeline\n");	
		return EXIT_FAILURE;
	}
    return 0;
}

/**
 * \brief start the streaming: set pipeling into PLAYING state
 * \param data of the stream (pipeline, bus and bust_watch_id) - see stream_data structure
 * \return 0 
 */
int start_streaming ( gpointer stream_datas ){
	stream_data *data 	=  stream_datas;
	struct videoFormatTable_entry * stream_entry = videoFormatTable_getEntry(data->videoFormatIndex);
  	/* Set the pipeline to "playing" state*/	
    g_print ("Now playing\n");
    gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
	stream_entry->videoFormatStatus = enable;
	return 0;
}

/**
 * \brief stop the streaming: set pipeling into NULL state
 * \param data of the stream (pipeline, bus and bust_watch_id) - see stream_data structure
 * \return 0 
 */
int stop_streaming( gpointer stream_datas ){
	stream_data *data 	=  stream_datas;
	struct videoFormatTable_entry * stream_entry = videoFormatTable_getEntry(data->videoFormatIndex);
	/* Out of the main loop, clean up nicely */	
	g_print ("Returned, stopping playback\n");
	gst_element_set_state (data->pipeline, GST_STATE_NULL);
	stream_entry->videoFormatStatus = disable;	
	return 0;
}

/**
 * \brief delete the stream: free pipeline and bus element
 * \param data of the stream (pipeline, bus and bust_watch_id) - see stream_data structure
 * \return 0 
 */
int delete_steaming_data( gpointer stream_datas ){
	stream_data *data 	=  stream_datas;	
	struct videoFormatTable_entry * stream_entry = videoFormatTable_getEntry(data->videoFormatIndex);
	/* delete pipeline */	
	g_print ("Deleting pipeline\n");
	stream_entry->videoFormatStatus = disable;		
	gst_object_unref (GST_OBJECT (data->pipeline));
	g_source_remove (data->bus_watch_id);
	return 0;
}


