/*
 * Licence: GPL
 * Created: Thu, 14 Jan 2016 12:05:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */

/* 
 * SubAgent Module
 */
#include <stdio.h>
#include <stdlib.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>



/* header file */
#include "../include/deamon.h"
#include "../include/streaming/stream.h"

/**
 * \brief the data needed to pass to functions used to exit the program nicely
 */
typedef struct{
	GMainLoop 			*loop;
	char 				*deamon_name;
	struct stream_data 	*stream_datas;
}stop_program_data;


static gboolean stop_program ( gpointer data ){
	stop_program_data *stop_data = data;
	GMainLoop *loop = (GMainLoop *) stop_data->loop;

	/* Stop net snmp subAgetnt deamon */
	snmp_shutdown(stop_data->deamon_name);

	stop_streaming(stop_data->stream_datas);

	g_main_loop_quit (loop);
	g_main_loop_unref (loop);	
	return TRUE;	
}



int main (int   argc,  char *argv[]){

	/* create the GMainLoop*/
	GMainLoop 	*loop;  	
	loop = g_main_loop_new (NULL, FALSE);

	/* add the idle function that handle SNMP request */
	g_timeout_add (10, handle_snmp_request, NULL);

	/* watch for stop signal */
	stream_data 	stream_datas;

	stop_program_data 		stop_data;
	stop_data.loop 			= loop;
	stop_data.deamon_name 	= argv[0];
	stop_data.stream_datas 	= &stream_datas;

	/* Exit the program nicely when kill signals are received */
	g_unix_signal_add (SIGINT,stop_program, &stop_data);
	g_unix_signal_add (SIGTERM,stop_program, &stop_data);

	/* init SubAgent Deamon */
	deamon(argv[0]);
	

	/* prepare the stream - initialize all the data relevant to the stream into stream-data */
	start_streaming(argc, argv, loop, &stream_datas);

	/* start the program: SNMP SubAgent deamon, and streaming */
    /* Iterate */	
	g_main_loop_run (loop);
	

	return 0;
}

