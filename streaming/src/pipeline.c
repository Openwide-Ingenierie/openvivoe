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
#include "../include/pipeline.h"
#include "../include/filter.h"


/* This function create the VIVOE pipeline for RAW videos
 * On the server side (udpsink)
 * Return TRUE is pipeline succed to be constructed, false otherwise
 */
gboolean raw_pipeline(GstElement*pipeline, GstBus *bus,
						guint bus_watch_id, GMainLoop *loop,
						GstElement* input, char* ip, gint port){
	
	/*Create element that will be add to the pipeline */
	GstElement *rtp, *udpsink;
	
	/* For Raw video */
	rtp = gst_element_factory_make ("rtpvrawpay", "rtp");
	if( rtp == NULL){
		g_printerr ( "error cannot create element for: %s\n","rtp");
		FALSE;        
	}
	
	/* Create the UDP sink */
    udpsink = gst_element_factory_make ("udpsink", "udpsink");
    if(udpsink == NULL){
       g_printerr ( "error cannot create element for: %s\n","udpsink");
	   return FALSE;
    }
    /*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsink),
                    "host", ip,
                    "port", port,
                    NULL);

    /* add elements to pipeline (and bin if necessary) before linking them */
    gst_bin_add_many(GST_BIN (pipeline),
                     rtp, 
                     udpsink,
                     NULL);

	/* Filters out non VIVOE videos, and link input to RTP if video has a valid format*/ 
	if (!filter_format(input, rtp)){
		return FALSE;
	}

	/* we link the elements together */
	if ( !gst_element_link_many (rtp, udpsink, NULL)){
		g_print ("Failed to link one or more elements for RAW streaming!\n");
	    return FALSE;
  	}else{
		return TRUE;
	}
}

/* This function create the VIVOE pipeline for MPEG-4 videos
 * On the server side (udpsink)
 */
int mp4_pipeline(GstElement*pipeline, GstBus *bus,
						guint bus_watch_id,GMainLoop *loop,
						GstElement* input, char* ip, gint port){
	
	/*Create element that will be add to the pipeline */
	GstElement *rtp, *udpsink;


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
                     rtp, 
                     udpsink,
                     NULL);

	/* Filters out non VIVOE videos */ 
	if (!filter_format(input, rtp)){
		EXIT_FAILURE;
	}

	/* we link the elements together */
	if ( !gst_element_link_many (rtp, udpsink, NULL)){
		g_print ("Failed to link one or more elements for MPEG-4 streaming!\n");
	    return EXIT_FAILURE;
  	}else{
		return EXIT_SUCCESS;
	}
}

