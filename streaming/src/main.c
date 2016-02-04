/*
 * Licence: GPL
 * Created: Thu, 28 Jan 2016 16:44:12 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */


#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
//#include <gstreamer-0.10/gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> 
#include "../include/streaming.h"

#define DEFAULT_IP "127.0.0.1" /*this is the address of the server to sink the UPD datagrams*/
#define DEFAULT_PORT 5004 /*this is the port number associated to the udp socket created for the sink*/
#define DEFAULT_FORMAT "raw" /*this is the default video format used for the stream */

#define MIN_DEFAULT_PORT 1024
#define MAX_DEFAULT_PORT 65535

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
/* 
 * This function is used to check the parameters used for intialize the stream
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

/* This function create the VIVOE pipeline for RAW videos
 * On the server side (udpsink)
 */
static int raw_pipeline(GstElement*pipeline, GstBus *bus,
						guint bus_watch_id, GMainLoop *loop,
						GstElement* source, char* ip, gint port){
	
	/*Create element that will be add to the pipeline */
	GstElement *rtp, *udpsink;

	/* For Raw video */
	rtp = gst_element_factory_make ("rtpvrawpay", "rtp");
	if( rtp == NULL){
		g_printerr ( "error cannot create element for: %s\n","rtp");
		EXIT_FAILURE;        
	}

	/* Create the UDP sink */
    udpsink = gst_element_factory_make ("udpsink", "udpsink");
    if(udpsink == NULL){
       g_printerr ( "error cannot create element for: %s\n","udpsink");
      return EXIT_FAILURE;        
    }
    /*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsink),
                    "host", ip,
                    "port", port,
                    NULL);

    /* add elements to pipeline (and bin if necessary) before linking them */
    gst_bin_add_many(GST_BIN (pipeline),
                     source,
                     rtp, 
                     udpsink,
                     NULL);

	/* we link the elements together */
	if ( !gst_element_link_many (source,rtp, udpsink, NULL)){
		g_print ("Failed to link one or more elements for RAW streaming!\n");
	    return EXIT_FAILURE;
  	}else{
		return EXIT_SUCCESS;
	}
}

/* This function create the VIVOE pipeline for MPEG-4 videos
 * On the server side (udpsink)
 */
static int mp4_pipeline(GstElement*pipeline, GstBus *bus,
						guint bus_watch_id,GMainLoop *loop,
						GstElement* source, char* ip, gint port){
	
	/*Create element that will be add to the pipeline */
	GstElement *rtp, *enc, *udpsink;

	 /*create the MPEG-4 encoder */
    enc = gst_element_factory_make ("avenc_mpeg4", "enc");
    if(enc == NULL){
       g_printerr ( "error cannot create element for: %s\n","enc");
      return EXIT_FAILURE;        
    }
	/* Create the RTP payload*/
    rtp = gst_element_factory_make ("rtpmp4vpay", "rtp");
    if(rtp == NULL){
       g_printerr ( "error cannot create element for: %s\n","rtp");
      return EXIT_FAILURE;        
    }
	
	/* Create the UDP sink */
    udpsink = gst_element_factory_make ("udpsink", "udpsink");
    if(udpsink == NULL){
       g_printerr ( "error cannot create element for: %s\n","udpsink");
      return EXIT_FAILURE;        
    }
    /*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsink),
                    "host", ip,
                    "port", port,
                    NULL);

    /* add elements to pipeline (and bin if necessary) before linking them */
    gst_bin_add_many(GST_BIN (pipeline),
                     source,
					 enc,
                     rtp, 
                     udpsink,
                     NULL);

	/* we link the elements together */
	if ( !gst_element_link_many (source,enc,rtp, udpsink, NULL)){
		g_print ("Failed to link one or more elements for MPEG-4 streaming!\n");
	    return EXIT_FAILURE;
  	}else{
		return EXIT_SUCCESS;
	}
}

int main (int   argc,  char *argv[])
{
    /* Initialization of elements needed */
    GstElementFactory *sourceFactory;
    GstElement *pipeline, *source;
    GstBus *bus;
    guint bus_watch_id;    
    GMainLoop *loop;
	gchar* ip = g_strdup( (gchar*) DEFAULT_IP);
	gint port 	= DEFAULT_PORT;
	char* format = strdup(DEFAULT_FORMAT);
	

	/* Initialize GStreamer */
    gst_init (&argc, &argv);
	/* Check input arguments */
	check_param(argc, argv, &ip, &port, &format);
    
    /* Initialize the main loop, (replace gst-launch) */
    loop = g_main_loop_new (NULL, FALSE);

    /* Create the pipeline */
    pipeline  = gst_pipeline_new ("pipeline");
	if(pipeline  == NULL){
		g_printerr ( "error cannot create: %s\n","pipeline" );
		return EXIT_FAILURE;        
	}

    /* Create the source, for now we use videosrctest */ 
    sourceFactory = gst_element_factory_find("videotestsrc");
    if(sourceFactory == NULL){
        g_printerr ( "error cannot create Factory for: %s\n","videotestsrc" );
       return EXIT_FAILURE;        
    }
    source = gst_element_factory_create (sourceFactory, "source");
    if(source == NULL){
       g_printerr ( "error cannot create element from factory for: %s\n", (char*) g_type_name (gst_element_factory_get_element_type (sourceFactory)));
      return EXIT_FAILURE;        
    }
	
	/* we add a message handler */
	bus = gst_pipeline_get_bus ( GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);
	
	if( !strcmp(format, "mp4")){
		/* Create the VIVOE pipeline for MPEG-4 videos */	
		if( mp4_pipeline(pipeline, bus, bus_watch_id,loop, source,  ip,  port)){
			g_printerr ( "Failed to create pipeline for MPEG-4 video\n");
			return EXIT_FAILURE; 
		}
	}else{
		/* Create the VIVOE pipeline for RAW videos */	
		if(raw_pipeline(pipeline, bus, bus_watch_id,loop, source,  ip,  port)){
			g_printerr ( "Failed to create pipeline for RAW video\n");
			return EXIT_FAILURE; 
		}
	}
    /* Set the pipeline to "playing" state*/
    g_print ("Now playing\n");
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);


    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);
    return 0;
}
