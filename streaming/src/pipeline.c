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

/* pad templates */

#define VIDEO_SRC_RGB_CAPS \
    GST_VIDEO_CAPS_MAKE("{ RGB, RGBx, xRGB, RGBA, ARGB, " \
						   "YV12, YUY2, UYVY, AYUV, YUV }")
/* This function create the VIVOE pipeline for RAW videos
 * On the server side (udpsink)
 */
int raw_pipeline(GstElement*pipeline, GstBus *bus,
						guint bus_watch_id, GMainLoop *loop,
						GstElement* source, char* ip, gint port){
	
	/*Create element that will be add to the pipeline */
	GstElement *rtp, *udpsink;
	
	/* Create a Caps Filter 
	 * This CapsFilter will be used to limit the 
	 * video caps in the source pad of RTP payload
	 * to video format support by VIVOE*/
	GstCaps *filter = gst_caps_from_string (VIDEO_SRC_RGB_CAPS);
/*	 GstCapsFilter* capsfilter;
	g_object_set( 	G_OBJECT(capsfilter), 
					"caps", filter,
					NULL);
*/


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
                     /*source,*/
					 /*capsfilter,*/
                     rtp, 
                     udpsink,
                     NULL);
	if ( !gst_element_link_filtered(source, rtp, filter)){
		g_print ("Failed to link one or more elements for RAW streaming!\n");
	    return EXIT_FAILURE;
	}
	/* we link the elements together */
	if ( !gst_element_link_many (/*source,*/rtp, udpsink, NULL)){
		g_print ("Failed to link one or more elements for RAW streaming!\n");
	    return EXIT_FAILURE;
  	}else{
		return EXIT_SUCCESS;
	}
}

/* This function create the VIVOE pipeline for MPEG-4 videos
 * On the server side (udpsink)
 */
int mp4_pipeline(GstElement*pipeline, GstBus *bus,
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

