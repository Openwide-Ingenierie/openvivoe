/*
 * Licence: GPL
 * Created: Mon, 08 Feb 2016 09:32:44 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */


#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gst/video/video.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/filter.h"

/* 
 * This function aims to filter out input videos which do not 
 * belong to the set of VIVOE's videos
 * returns TRUE if the video is filter out, false otherwise
 * */
gboolean filter_raw(GstElement* input, GstElement* output){
	
	/* Create a Caps Filter 
	 * This CapsFilter will be used to limit the 
	 * video caps in the input pad of RTP payload
	 * to video format support by VIVOE*/
	printf("%s\n", VIVOE_RAW_CAPS);
	GstCaps *filter = gst_caps_from_string (VIVOE_RAW_CAPS );
	if ( !gst_element_link_filtered(input, output ,filter)){
		g_print ("Failed to link one or more elements for RAW streaming!\n");
	    return FALSE;
	}else{
		return TRUE;	
	}
}


gboolean filter_mp4(GstElement* input, GstElement* output){
	
	/* Create a Caps Filter 
	 * This CapsFilter will be used to limit the 
	 * video caps in the input pad of RTP payload
	 * to video format support by VIVOE*/
	GstCaps *filter = gst_caps_from_string (VIVOE_MPEG4_CAPS );
	if ( !gst_element_link_filtered(input, output ,filter)){
		g_print ("Failed to link one or more elements for RAW streaming!\n");
	    return FALSE;
	}else{
		return TRUE;	
	}
}
