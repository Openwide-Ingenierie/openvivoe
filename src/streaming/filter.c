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
#include "../../include/streaming/filter.h"

/* 
 * This function aims to filter out input videos which do not 
 * belong to the set of VIVOE's videos
 * returns TRUE if the video is filter out, false otherwise
 * */
gboolean filter_VIVOE(GstElement* input, GstElement* output, int format){
	GstCaps *filter;	
	/* Create a Caps Filter 
	 * This CapsFilter will be used to limit the 
	 * video caps in the input pad of RTP payload
	 * to video format support by VIVOE*/
	if( format == RAW_FILTER ){
		filter = gst_caps_from_string (VIVOE_RAW_CAPS );
	}else if (format == MP4_FILTER) { //assume it is mp4 format
		filter = gst_caps_from_string (VIVOE_MPEG4_CAPS );
	}else{
		g_printerr("Wrong format have been specified for filtering\n");
		return FALSE;
	}
	if ( !gst_element_link_filtered(input, output ,filter)){
		g_print ("Failed to link one or more elements for RAW streaming!\n");
		return FALSE;
	}else{
		return TRUE;	
	}
}
