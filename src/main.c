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
#include "../include/daemon.h"
#include "../include/mibParameters.h"
#include "../include/videoFormatInfo/videoFormatTable.h"
#include "../include/channelControl/channelTable.h"
#include "../include/multicast.h"
#include "../include/streaming/stream_registration.h"
#include "../include/streaming/stream.h"



#include "../include/announcement/sap.h"

/**
 * \brief the data needed to pass to functions used to exit the program nicely
 */
typedef struct{
	GMainLoop 			*loop; 			/* the main Loop to quit and unref */
	char 				*deamon_name; 	/* the deamon's nam to stop */
}stop_program_data;

/**
 * \brief the data needed to pass to functions used to exit the program nicely
 * \param data the data we need to know to exit the program nicely see stop_program_data
 * \return TRUE
 */
static gboolean stop_program ( gpointer data ){
	stop_program_data *stop_data = data;
	GMainLoop *loop = (GMainLoop *) stop_data->loop;

	/* Stop net snmp subAgetnt deamon */
	snmp_shutdown(stop_data->deamon_name);
	/* delete all streams associated to all channels */
	struct channelTable_entry *iterator = channelTable_head;
	struct channelTable_entry *temp;
	while(iterator != NULL){
		temp = iterator;
		iterator = iterator->next;
		if ( temp->channelType == videoChannel )
			delete_steaming_data(temp);
	}
	g_main_loop_quit (loop);
	g_main_loop_unref (loop);
	return TRUE;
}

static gboolean service_Provider_init(gpointer loop){
	/* data associated to stream */	
/*	stream_data 	stream1;
	stream_data 	stream2;*/

	/* prepare the stream - initialize all the data relevant to the stream into stream-data */
	if ( init_streaming(loop, NULL, NULL,/*&stream1, test*/ "RAW", 1920, 1080 /*end test param*/)){
		return FALSE;
	}

	/* prepare the stream - initialize all the data relevant to the stream into stream-data */
	if ( init_streaming(loop, NULL, NULL,/*&stream2, test*/ "MP4V-ES", 1920, 1080 /*end test param*/)){
		return  FALSE;
	}

	return TRUE;
}

/**
 * \brief the data needed to pass to functions used to exit the program nicely
 * \param data the data we need to know to exit the program nicely see stop_program_dat
 * \return 0 as it's our main function
 */
int main (int   argc,  char *argv[]){
	/* create the GMainLoop*/
	loop = g_main_loop_new (NULL, FALSE);

	/* add the idle function that handle SNMP request every 100ms */
	g_timeout_add (10, handle_snmp_request, NULL);

	/* init SubAgent Deamon */
	open_vivoe_daemon (argv[0]);

	/* In case of an service Provider */
	/* Initialize GStreamer */
	gst_init (&argc, &argv);

	/* data associated to stream */
	stop_program_data 		stop_data;
	stop_data.deamon_name 	= argv[0];
	stop_data.loop 			= loop;

	/* Exit the program nicely when kill signals are received */
	g_unix_signal_add (SIGINT, 	stop_program, &stop_data);
	g_unix_signal_add (SIGTERM, stop_program, &stop_data);

	/* start the program: SNMP SubAgent deamon, and streaming */
	if ( 	deviceInfo.parameters[num_DeviceType]._value.int_val == device_SP
	  	 || deviceInfo.parameters[num_DeviceType]._value.int_val == device_both){
		if ( !service_Provider_init( loop )) {
			g_printerr("Failed to load streams\n");
			return 1;
		}
	}


    /* Iterate */
	g_main_loop_run (loop);
	
	return 0;
}
