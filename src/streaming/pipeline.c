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
#include "../../include/streaming/pipeline.h"
#include "../../include/streaming/filter.h"
#include "../../include/streaming/detect.h"


/* This function create the VIVOE pipeline for RAW videos
 * On the server side (udpsink)
 * Return TRUE is pipeline succed to be constructed, false otherwise
 */
gboolean raw_pipeline( 	GstElement*pipeline, 	GstBus *bus,
						guint bus_watch_id, GMainLoop *loop,
						GstElement* input, 	char* ip, gint port){
	
	/*Create element that will be add to the pipeline */
	GstElement *rtp, *udpsink;
	const char* mib_caps; /* use to save the caps of the video after payloading, and before udpsink */

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

	/* add rtp to pipeline */
	gst_bin_add_many(GST_BIN (pipeline), 
					 rtp,
					 udpsink,
					 NULL);
	/* Filters out non VIVOE videos, and link input to RTP if video has a valid format*/ 
	if (!filter_raw(input, rtp)){
		return FALSE;
	}

	/* Media stream Type detection */
	mib_caps = strdup (type_detection(GST_BIN(pipeline), rtp, loop));
	
	/* Create the VIVOE pipeline for MPEG-4 videos */
	GstCaps 		*detected 		= gst_caps_from_string(mib_caps);
	GstStructure 	*str_detected 	= gst_caps_get_structure(detected, 0);
	
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
int mp4_pipeline( 	GstElement*pipeline, 	GstBus *bus,
					guint bus_watch_id, 	GMainLoop *loop,
					GstElement* input, 		char* ip, gint port){
	
	/*Create element that will be add to the pipeline */
	GstElement *rtp, *udpsink;
	const char* mib_caps; /* use to save the caps of the video after payloading, and before udpsink */

	/* Create the RTP payload*/
    rtp = gst_element_factory_make ("rtpmp4vpay", "rtp");
    if(rtp == NULL){
       g_printerr ( "error cannot create element for: %s\n","rtp");
      return FALSE;        
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

	/* Filters out non VIVOE videos */ 
	if (!filter_mp4(input, rtp)){
		FALSE;
	}
	
	/* Media stream Type detection */
	mib_caps = strdup (type_detection(GST_BIN(pipeline), rtp, loop));
	
	/* Create the VIVOE pipeline for MPEG-4 videos */
	GstCaps 		*detected 		= gst_caps_from_string(mib_caps);
	GstStructure 	*str_detected 	= gst_caps_get_structure(detected, 0);

	/* we link the elements together */
	if ( !gst_element_link_many (rtp, udpsink, NULL)){
		g_print ("Failed to link one or more elements for MPEG-4 streaming!\n");
	    return FALSE;
  	}else{
		return TRUE;
	}
}

