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
#include "../include/mibParameters.h"
#include "../include/deviceInfo/ethernetIfTable.h"
#include "../include/videoFormatInfo/videoFormatTable.h"
#include "../include/channelControl/channelTable.h"
#include "../include/announcement/sap.h"
#include "../include/multicast.h"
#include "../include/streaming/plugin/vivoecrop/gstvivoecrop.h"
#include "../include/streaming/plugin/vivoecaps/vivoecaps.h"
#include "../include/streaming/stream_registration.h"
#include "../include/streaming/stream.h"
#include "../include/daemon.h"

/**
 * \brief the data needed to pass to functions used to exit the program nicely
 */
typedef struct{
	GMainLoop 			*loop; 				/* the main Loop to quit and unref */
	char 				*deamon_name; 		/* the deamon's nam to stop */
}stop_program_data;

/**
 * \brief the data needed to pass to functions used to exit the program nicely
 * \param data the data we need to know to exit the program nicely see stop_program_data
 * \return TRUE
 */
static gboolean stop_program ( gpointer data ){
	stop_program_data *stop_data = data;
	GMainLoop *loop = (GMainLoop *) stop_data->loop;

	/* Stop net snmp subAgent deamon */
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
	free(deviceInfo.parameters);
	/* free tables */
	/* channel table */
	channelTable_delete();
	/* videoFormatTable */
	videoFormatTable_delete();
	/* ethernetIfTable */
	ethernetIfTableEntry_delete();
	/* unref and quit main loop */
	g_main_loop_quit (loop);
	g_main_loop_unref (loop);

	/* to exit the program we need to reset was_running variable */
	exit(EXIT_SUCCESS);

	/* this is for the prototype, we should not get there */
	return TRUE;
}

/**
 * \brief iterate throug the channel table, set to "start" all SU channels so they start to listen for SAP/SDP annoucnement
 * \param loop the GMainLoop
 */
static void default_startUp_mode(gpointer loop){
	for (int i=1; i<=channelNumber._value.int_val; i++){
		struct channelTable_entry * entry 	= channelTable_getEntry(i);
		if ( entry->channelType == serviceUser && entry->channelDefaultReceiveIpAddress ){
			entry->channelStatus 			= channelStart;
			entry->channelReceiveIpAddress 	= entry->channelDefaultReceiveIpAddress;
			channelSatus_requests_handler( entry );
		}
	}
}

/*
 * \brief initialize the boolean "internal_error" to false before running the main loop
 */
gboolean internal_error = FALSE;

/*
 * \brief initialize the boolean "was_runnig" to false before running the main loop
 */
gboolean was_running = FALSE;

gboolean vivoe_gstreamer_initiation(int   argc,  char *argv[]){

	gst_init (&argc, &argv);

	/*
	 * register vivoecrop plugin
	 */
	if (  (! vivoecrop_init ()) || (! vivoecaps_init ()) )
		g_error("Failed to load OpenVivoe's private gstreamer's modules");
	else
		return TRUE;

}

/**
 * \brief the data needed to pass to functions used to exit the program nicely
 * \param argc the number of argument
 * \param argv argument given to the program
 * \return 0 as it's our main function
 */
int main (int   argc,  char *argv[]){

	/* create the GMainLoop*/
	main_loop = g_main_loop_new (NULL, FALSE);

	g_message("starting the program");

	/* data associated to stream */
	stop_program_data 			stop_data;
	stop_data.deamon_name 		= argv[0];
	stop_data.loop 				= main_loop;

	/* Exit the program nicely when kill signals are received */
	g_unix_signal_add (SIGINT, 	stop_program, &stop_data);
	g_unix_signal_add (SIGTERM, stop_program, &stop_data);

	/* In case of an service Provider */
	/* Initialize GStreamer */
	if ( !vivoe_gstreamer_initiation( argc,  argv) )
		return EXIT_FAILURE;

	/* init SubAgent Deamon */
	if ( open_vivoe_daemon (argv[0]) )
		return EXIT_FAILURE;

	/* add the idle function that handle SNMP request every 100ms */
	g_timeout_add (100, handle_snmp_request, NULL);

	/* checking for start up mode */
	if ( deviceInfo.parameters[num_DeviceMode]._value.int_val == defaultStartUp)
		default_startUp_mode(main_loop);

	/* listen to SAP/SDP announcement */
	if ( 	deviceInfo.parameters[num_DeviceType]._value.int_val == device_SU
	  	 || deviceInfo.parameters[num_DeviceType]._value.int_val == device_both)
		g_timeout_add(1000, receive_announcement, NULL);

	/* iterate */
	do {
		/* Iterate */
		if ( !internal_error){
			was_running = TRUE;
			g_main_loop_run (main_loop);
		}
		else{
			g_error("A Gstreamer's error occurs");
			return EXIT_FAILURE;
		}

	}while(was_running);

	return EXIT_SUCCESS;

}

