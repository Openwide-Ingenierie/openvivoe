/*
 * Licence: GPL
 * Created: Thu, 28 Jan 2016 16:44:12 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */


#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> 
#include "../include/streaming.h"

#define IP "127.0.0.1" /*this is the address of the server to sink the UPD datagrams*/
#define PORT 5004 /*this is the port number associated to the udp socket created for th sink*/

#define MIN_PORT 1024
#define MAX_PORT 65535

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
static int check_param(int argc, char* argv[], char** ip, gint* port){

	/* this will be used to check wether the IP address has a goot format or not */
	struct in_addr* check_addr = malloc(sizeof(struct in_addr));
	if (argc != 3) {
		g_printerr ("Usage: %sip=ddd.ddd.ddd.ddd port=[1024,65535]\n", argv[0]);
		g_print ("Default settings taken: ip=%s port=%d\n",(char*) *ip, *port);
		return EXIT_FAILURE;
	/*Check that parameters are indeed: ip= and port= */	
	}else if( strncmp("ip=", argv[1], strlen("ip=")) || strncmp("port=", argv[2], strlen("port="))) {
		g_printerr ("Usage: %s ip=ddd.ddd.ddd.ddd port=[1024,65535]\n", argv[0]);		
		g_print ("Default settings taken: ip=%s port=%d\n",(char*) *ip, *port);
		return EXIT_FAILURE;
	}
	/*Here, the fromat of the command line entered is good, we just need te check the format of the IP Address given, and the port number */

	/* First check IP address format */
	char* temp_ip = strdup(argv[1] + strlen("ip="));
	if (! inet_pton(AF_INET, temp_ip, check_addr)){
		g_printerr ("Ip should be Ipv4 dotted-decimal format\n");
		g_print ("Default settings taken: ip=%s port=%d\n",(char*) *ip, *port);
		return EXIT_FAILURE;
	}

	/* Then, chek port number ( should be greater than 1024 and less than 65535 as it is coded on a short int (16-bits) */
	char* temp_port = strdup(argv[2] + strlen("port="));
	int temp_numport = atoi(temp_port);
	if(temp_numport< MIN_PORT || temp_numport>MAX_PORT){
		g_printerr ("Port number should in the range: %d -> %d\n", MIN_PORT, MAX_PORT);
		g_print ("Default settings taken: ip=%s port=%d\n",(char*) *ip, *port);
		return EXIT_FAILURE;
	}
	/* replace ip's value by the value entered by the user */
	strcpy(*ip, temp_ip);
	*port = (gint) temp_numport;
	return EXIT_SUCCESS;
}

int main (int   argc,  char *argv[])
{
    /* Initialization of elements needed */
    GstElementFactory *sourceFactory;
    GstElement *pipeline, *source, *rtp, *udpsink;
    GstBus *bus;
    guint bus_watch_id;    
    GMainLoop *loop;
	gchar* ip = g_strdup( (gchar*) IP);
	gint port 	= PORT;
	
	/* Initialize GStreamer */
    gst_init (&argc, &argv);
	/* Check input arguments */
	check_param(argc, argv, &ip, &port);
    
    /* Initialize the main loop, (replace gst-launch) */
    loop = g_main_loop_new (NULL, FALSE);
    
    /* Create the pipeline */
    pipeline  = gst_pipeline_new ("pipeline");
	if(pipeline  == NULL){
		g_printerr ( "error cannot create: %s\n","pipeline" );
		EXIT_FAILURE;        
	}

    /* Create the source, for now we use videosrctest */ 
    sourceFactory = gst_element_factory_find("videotestsrc");
    if( sourceFactory == NULL){
        g_printerr ( "error cannot create Factory for: %s\n","videotestsrc" );
        EXIT_FAILURE;        
    }
    source = gst_element_factory_create (sourceFactory, "source");
    if( source == NULL){
       g_printerr ( "error cannot create element from factory for: %s\n", (char*) g_type_name (gst_element_factory_get_element_type (sourceFactory)));
       EXIT_FAILURE;        
    }
   
    rtp = gst_element_factory_make ("rtpvrawpay", "rtp");
    if( rtp == NULL){
       g_printerr ( "error cannot create element for: %s\n","rtp");
       EXIT_FAILURE;        
    }
    
    udpsink = gst_element_factory_make ("udpsink", "udpsink");
    if( udpsink == NULL){
       g_printerr ( "error cannot create element for: %s\n","udpsink");
       EXIT_FAILURE;        
    }
    
    g_object_set(   G_OBJECT(udpsink),
                    "host", ip,
                    "port", port,
                    NULL);

    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

    /* add elements to pipeline (and bin if necessary) before linking them */
    gst_bin_add_many(GST_BIN (pipeline),
                     source,
                     rtp, 
                     udpsink,
                     NULL);
    
    /* we link the elements together */
    /* videosrctest -> parse ("rtpvrawpay ! udpsink host=127.0.0.1 port=1993") */
    gst_element_link_many (source, /*parse*/ rtp, udpsink, NULL);
   
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
