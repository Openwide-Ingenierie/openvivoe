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
#include "../../include/conf/stream-conf.h"
#include "../../include/streaming/filter.h"

	/* open configuration file */
	GKeyFile*  gkf = open_configuration_file();
	if(gkf == NULL)
		return NULL; /* if not configuration file was found, return NULL, meaning that we will be using the defaults filter for VIVOE formats*/	
	GstCaps* final_filer = NULL	
	/* close configuration file */
	close_configuration_file(gkf);;
	
/*
 * Build the RAW filter caps
 */
GstCaps* build_RAW_filter(GKeyFile* gkf)){
	gchar** 		encoding;
	gchar** 		resolution;
	GstCaps* 		raw_filter = NULL;	
	encoding 	= get_raw_encoding(GKeyFile* gkf);
	resolution 	= get_raw_res(GKeyFile* gkf);
	
	/* Add encoding part */ 
	if( g_strv_contains(encoding, "all") ||
		( g_strv_contains(encoding, "RGB") && g_strv_contains(encoding, "YUV") && g_strv_contains(encoding, "Monochrome"))
	  )
	{
		raw_filter = gst_caps_from_string (VIVOE_FORMAT_RAW_ENCODING);
	}else{
		if( g_strv_contains(encoding, "RGB") ){
			raw_filter = gst_caps_from_string ("format = (string) { RGB, RGBA, BGR, BGRA }" );
		}
		if( g_strv_contains(encoding, "YUV") ){
			raw_filter = gst_caps_merge(raw_filter, ( gst_caps_from_string ("format = (string) { AYUV, UYVY, I420, Y41B, UYVP }" )));
		}
		if( g_strv_contains(encoding, "Monochrome") ){
		}
	}
	
	/* Add resolution part */
	if( g_strv_contains(encoding, "all") ||
		( g_strv_contains(encoding, "576i") && g_strv_contains(encoding, "720p") && g_strv_contains(encoding, "1080i") && g_strv_contains(encoding, "1080p") )
	  )
	{
		raw_filter = gst_caps_merge(raw_filter, ( gst_caps_from_string ()));
	}else{
}

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
