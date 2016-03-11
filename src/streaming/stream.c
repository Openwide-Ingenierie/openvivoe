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
								   break;
							   }
		default:
			break;
	}

	return TRUE;
}

#if 0
/**
 * \brief check the parameters used for intialize the stream
 * \param argc: as in main (should be 3)
 * \param argv: as in main (should begin with ip=... port=... format=...)
 * \return int EXIT_FAILURE if wrong parameters passed to the program, EXIT_SUCCESS otherwise
 */
static int check_param(int argc, char* argv[], char** ip, gint* port, char** format){

	/* this will be used to check wether the DEFAULT_IP address has a goot format or not */
	struct in_addr* check_addr = malloc(sizeof(struct in_addr));
	if (argc != 4) {
		g_printerr ("Usage: %s ip=ddd.ddd.ddd.ddd port=[1024,65535] format=[raw;mp4]\n", argv[0]);
		g_print ("Default settings taken: ip=%s port=%d format=%s\n",(char*) *ip, *port, *format);
		return EXIT_FAILURE;
	/*Check that parameters are indeed: ip= and port= */
	}else if( strncmp("ip=", argv[1], strlen("ip=")) || strncmp("port=", argv[2], strlen("port=")) || strncmp("format=", argv[3], strlen("format=")) ) {
		g_printerr ("Usage: %s ip=ddd.ddd.ddd.ddd port=[1024,65535] format=[raw;mp4]\n", argv[0]);
		g_print ("Default settings taken: ip=%s port=%d format=%s\n",(char*) *ip, *port, *format);
		return EXIT_FAILURE;
	}
	/*Here, the fromat of the command line entered is good, we just need te check the format of the DEFAULT_IP Address given, and the port number */

	/* First check DEFAULT_IP address format */
	char* temp_ip = strdup(argv[1] + strlen("ip="));
	if (! inet_pton(AF_INET, temp_ip, check_addr)){
		g_printerr ("Ip should be Ipv4 dotted-decimal format\n");
		g_print ("Default settings taken: ip=%s port=%d format=%s\n",(char*) *ip, *port, *format);
		return EXIT_FAILURE;
	}

	/* Then, chek port number ( should be greater than 1024 and less than 65535 as it is coded on a short int (16-bits) */
	char* temp_port = strdup(argv[2] + strlen("port="));
	int temp_numport = atoi(temp_port);
	if(temp_numport< MIN_DEFAULT_PORT || temp_numport>MAX_DEFAULT_PORT){
		g_printerr ("Port number should in the range: %d -> %d\n", MIN_DEFAULT_PORT, MAX_DEFAULT_PORT);
		g_print ("Default settings taken: ip=%s port=%d format=%s\n",(char*) *ip, *port, *format);
		return EXIT_FAILURE;
	}

	/*Then, check the value of the format given should be raw  or mp4 */
	char* temp_format = strdup(argv[3] + strlen("format="));
	if( strcmp("raw", temp_format) &&  strcmp("mp4", temp_format) ){
		g_printerr ("Format should be: %s or %s\n", "raw", "mp4");
		g_print ("Default settings taken: ip=%s port=%d format=%s\n",(char*) *ip, *port, *format);
		return EXIT_FAILURE;
	}

	/* replace ip's value by the value entered by the user */
	strcpy(*ip, temp_ip);
	*port = (gint) temp_numport;
	strcpy(*format, temp_format);
	return EXIT_SUCCESS;
}
#endif //if 0

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
	source = gst_element_factory_make ("videotestsrc", "source");
	if(source == NULL){
		g_printerr ("error cannot create element: %s\n", "videotestsrc" );
		return NULL;
	}
	g_object_set(source, "is-live", TRUE, NULL);
	capsfilter = gst_element_factory_make ("capsfilter","capsfilter");
	if(capsfilter == NULL){
		g_printerr ( "error cannot create element: %s\n", "capsfilter" );
		return NULL;
	}

	caps = gst_caps_new_full( 	gst_structure_new( 	"video/x-raw" 	,
													//"format" 		, G_TYPE_STRING , encoding,
													"width" 		, G_TYPE_INT 	, width,
													"height" 		, G_TYPE_INT 	, height,
													"interlace-mode", G_TYPE_STRING , "progressive",
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
	return last;
}

static int init_stream_SP( gpointer main_loop, gpointer stream_datas /* real prototype */
					, char* format, int width, int height/* extra parameters for testing purposes*/)
{
	stream_data *data 			= stream_datas;

	GstElement 	*pipeline 		= data->pipeline;

    GstElement *last;	
	/* Source Creation */
	last = source_creation(pipeline, format,width ,height /*,encoding*/);

	if (last == NULL ){
		g_printerr ( "Failed to create videosource\n");
		return EXIT_FAILURE;
	}
	/* Create pipeline  - save videoFormatIndex into stream_data data*/
	last = create_pipeline_videoChannel( stream_datas, 	main_loop, last );
	/* Check if everything went ok*/
	if (last == NULL){
		g_printerr ( "Failed to create pipeline\n");
		return EXIT_FAILURE;
	}
    return EXIT_SUCCESS;
}

static int init_stream_SU( gpointer main_loop, gpointer stream_datas, GstCaps *caps, struct channelTable_entry *channel_entry )
{
    GstElement *first;		
	/* Create pipeline  - save videoFormatIndex into stream_data data*/
	first = create_pipeline_serviceUser( stream_datas,	main_loop, caps, channel_entry );
	/* Check if everything went ok*/
	if (first == NULL){
		g_printerr ( "Failed to create pipeline\n");
		return EXIT_FAILURE;
	}
    return EXIT_SUCCESS;
}

/**
 * \brief intialize the stream: create the pipeline and filters out non vivoe format
 * \param argc to know which source to built
 * \param argv to know which source to built
 * \param stream_datas a structure in which we will save the pipeline and the bus elements
 * \return 0
 */
int init_streaming (gpointer main_loop, GstCaps *caps, struct channelTable_entry * channel_entry,/* real prototype */
					char* format, int width, int height/* extra parameters for testing purposes*/)
{
    /* Initialization of elements needed */
    GstElement 	*pipeline;
    GstBus 		*bus;
    guint 		bus_watch_id;
	GMainLoop 	*loop 			= main_loop;


	/* Reference all data relevant to the stream */
	stream_data *data 			= malloc(sizeof(stream_data));

	/* allocate memory for the structure rtp_data of stream_data */
	/* this will be free in the delete_stream function */
	rtp_data 	*rtp_datas 		= malloc(sizeof(rtp_data));
	data->rtp_datas 			= rtp_datas;
	
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

	data->pipeline 		= pipeline;
	data->bus 			= bus;
	data->bus_watch_id 	= bus_watch_id;
	if( format != NULL){ /* case we are a Service Provider */
		 init_stream_SP( main_loop, data, format, width, height);
		return EXIT_SUCCESS;
	}
	else{ /* case we are a Service User */
		init_stream_SU( main_loop, data, caps, channel_entry);
		channel_entry->stream_datas = data;
		return EXIT_SUCCESS;
	}
}

/**
 * \brief start the streaming: set pipeling into PLAYING state
 * \param data of the stream (pipeline, bus and bust_watch_id) - see stream_data structure
 * \return 0
 */
int start_streaming (gpointer stream_datas, long channelVideoFormatIndex ){
	stream_data *data 								= stream_datas;
	struct videoFormatTable_entry * stream_entry 	= videoFormatTable_getEntry(channelVideoFormatIndex);
  	/* Set the pipeline to "playing" state*/
    g_print ("Now playing\n");
    gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
	if ( stream_entry != NULL)
		stream_entry->videoFormatStatus = enable;
	return 0;
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
		gst_element_set_state (data->pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (data->pipeline));
		g_source_remove (data->bus_watch_id);
		/* free rtp_data */
		free(data->rtp_datas);
		/* free the sap_data */
		if( entry->channelType == videoChannel)
			free(entry->sap_datas);
		free(data); 
	}

if( entry->channelType == videoChannel)
	free(entry);
	return 0;
}
