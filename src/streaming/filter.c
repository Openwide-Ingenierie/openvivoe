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

/*
 * return TRUE if all encoding are used 
 */
static gboolean all_encodings(const gchar * const * encoding){
	return (g_strv_contains(encoding, "all") ||
			( g_strv_contains(encoding, "RGB") && 
			  g_strv_contains(encoding, "YUV") && 
			  g_strv_contains(encoding, "Monochrome")));
}

/*
 * return TRUE if all resolutions are used 
 */
static gboolean all_resolutions(const gchar * const * resolution){
	return (g_strv_contains(resolution, "all") ||
			( g_strv_contains(resolution, "576i") 	&& 
			  g_strv_contains(resolution, "720p") 	&& 
			  g_strv_contains(resolution, "1080i") 	&& 
			  g_strv_contains(resolution, "1080p") ));
}


/*
 * This function build the default GValue for widths 
 * iparameters taken are the indexes of desired min and max width
 */
/* local define for the array of widths */
static GValue build_list(int* int_array, gchar** char_array, int length_array ){
	GValue resolution = G_VALUE_INIT;
	g_value_init (&resolution, GST_TYPE_LIST);
	GValue item = G_VALUE_INIT;
	if( int_array != NULL){
		g_value_init (&item, G_TYPE_INT);
		/* Build list of values */
		for(int i=0; i < length_array; i++){
			g_value_set_int (&item, int_array[i]);
			gst_value_list_append_value (&resolution, &item);
		}
	}
	else if ( char_array != NULL){
		g_value_init (&item, G_TYPE_STRING);
		/* Build list of values */
		for(int i=0; i < length_array; i++){
			g_value_set_static_string	(&item, char_array[i]);
			gst_value_list_append_value (&resolution, &item);
		}
	}
	else{
		g_printerr("probleme while building list\n");
	}
	return resolution;
}


static void set_filter_field( 	GstStructure* gst,
								const gchar* field,
								int* int_array,
								gchar** char_array,
								int length_array,
							   	const gchar* value,
								GType type){
	GValue set = G_VALUE_INIT;			
	/* if Value is NULL, we are initializing a list of values */
	if( value == NULL){
		g_value_init (&set,GST_TYPE_LIST);
		set = build_list(int_array, char_array,length_array);
	}
	else{
		switch(type){
			case G_TYPE_STRING: 
				g_value_set_static_string (&set, value );
				break;
			case G_TYPE_INT:
				g_value_set_int (&set, atoi(value));
				break;
			default:
				/*this is bad, we should never get here*/
				break;
		}
	}
	gst_structure_set_value(gst,
                        	field,
							&set);
}

/*
 * Build the RAW filter caps
 */
static GstStructure* build_RAW_filter(GKeyFile* gkf){
	const gchar * const * 		encoding 	= (const gchar * const *)	get_raw_encoding(gkf);
	const gchar * const * 		resolution 	= (const gchar * const *)	get_raw_res(gkf);
	GstStructure* 				raw_filter 	= gst_structure_new_from_string ("video/x-raw");

	/* Add encoding part */ 
	if( all_encodings(encoding) ){
		char* formats[12]= { 	"RGB", "RGBA", "BGR", "BGRA",
	   							"AYUV", "UYVY","I420", "Y41B", "UYVP",
								"GRAY8", "GRAY16_BE", "GRAY16_LE"};
		set_filter_field(raw_filter, "format", NULL, formats , 12, NULL, G_TYPE_GTYPE);
	}else{
		if( g_strv_contains(encoding, "RGB") ){
			char* RGB[4]= { "RGB", "RGBA", "BGR", "BGRA" };
			set_filter_field(raw_filter, "format",NULL, RGB ,4, NULL, G_TYPE_GTYPE);
		}
		if( g_strv_contains(encoding, "YUV") ){
			char* YUV[5]= { "AYUV", "UYVY"," I420", "Y41B", "UYVP" };
			set_filter_field(raw_filter, "format",NULL, YUV, 5, NULL,G_TYPE_GTYPE );			
		}
		if( g_strv_contains(encoding, "Monochrome") ){
			char* Monochrome[3]= {"GRAY8", "GRAY16_BE", "GRAY16_LE" };
			set_filter_field(raw_filter, "format",NULL, Monochrome, 3, NULL,G_TYPE_GTYPE );			
		}
	}
	
	/* Add resolution part */
	if( all_resolutions( resolution ) )
	{
		int width[6]={576, 704, 720, 768, 1280, 1920};
		int height[4]={544, 576, 720, 1080};
		set_filter_field(raw_filter, "width", 	width, 	NULL, 6, NULL, G_TYPE_GTYPE);
		set_filter_field(raw_filter, "height", 	height, NULL, 4, NULL, G_TYPE_GTYPE);
	}else{
		if( g_strv_contains(resolution,"576i") ){
			int width[3]= {544, 576, 720};
			int heigth[2]= {576, 704};
			set_filter_field(raw_filter, "width", 			width, 	NULL, 3, NULL, 			G_TYPE_GTYPE );
			set_filter_field(raw_filter, "height", 			heigth, NULL, 2, NULL, 			G_TYPE_GTYPE);
			set_filter_field(raw_filter,"interlace-mode", 	NULL, 	NULL, 0, "interleaved", G_TYPE_STRING);
		}
		if( g_strv_contains(resolution, "720p") ){
			set_filter_field(raw_filter, "width",  			NULL, NULL, 0, "1280",		 	G_TYPE_INT );
			set_filter_field(raw_filter, "height", 			NULL, NULL, 0, "720",			G_TYPE_INT );
			set_filter_field(raw_filter, "interlace-mode", 	NULL, NULL, 0, "progresive",  	G_TYPE_STRING);
		}
		if( g_strv_contains(resolution, "1080p") ){
			set_filter_field(raw_filter, "width", 			NULL, NULL, 0, "1920",		  	G_TYPE_INT );
			set_filter_field(raw_filter, "height", 			NULL, NULL, 0, "1080",		  	G_TYPE_INT );
			set_filter_field(raw_filter, "interlace-mode", 	NULL, NULL, 0, "progresive",  	G_TYPE_STRING);
		}
		if( g_strv_contains(resolution, "1080i") ){
			set_filter_field(raw_filter, "width", 			NULL, NULL, 0, "1920",		  	G_TYPE_INT );
			set_filter_field(raw_filter, "height", 			NULL, NULL, 0, "1080",		  	G_TYPE_INT );
			set_filter_field(raw_filter, "interlace-mode", 	NULL, NULL, 0, "interleaved", 	G_TYPE_STRING);
		}
	}
//	set_filter_field(raw_filter, "framerate", GST_VIDEO_FPS_RANGE);
	printf("%s\n", gst_structure_to_string (raw_filter) );	
	return raw_filter;
}
/*
 * Build the MPEG-4 filter caps
 */
static GstStructure* build_MPEG4_filter(GKeyFile* gkf){
	const gchar * const * 		resolution 	= (const gchar * const *)	get_raw_res(gkf);
	GstStructure* 				mpeg_filter = gst_structure_new_from_string ("video/mpeg, mpegversion=(int)4");	
	
	/* Add resolution part */
	if( all_resolutions( resolution ) )
	{
		int width[6]={576, 704, 720, 768, 1280, 1920};
		int height[4]={544, 576, 720, 1080};
		set_filter_field(mpeg_filter, "width", 	width, 	NULL, 6, NULL, G_TYPE_GTYPE);
		set_filter_field(mpeg_filter, "height",	height, NULL, 4, NULL, G_TYPE_GTYPE);
	}else{
		if( g_strv_contains(resolution,"576i") ){
			int width[3]= {544, 576, 720};
			int heigth[2]= {576, 704};
			set_filter_field(mpeg_filter, "width", 			width, 	NULL, 3, NULL, 			G_TYPE_GTYPE );
			set_filter_field(mpeg_filter, "height", 		heigth, NULL, 2, NULL, 			G_TYPE_GTYPE);
			set_filter_field(mpeg_filter,"interlace-mode", 	NULL, 	NULL, 0, "interleaved", G_TYPE_STRING);
		}
		if( g_strv_contains(resolution, "720p") ){
			set_filter_field(mpeg_filter, "width",  		NULL, NULL, 0, "1280",		 	G_TYPE_INT );
			set_filter_field(mpeg_filter, "height", 		NULL, NULL, 0, "720",			G_TYPE_INT );
			set_filter_field(mpeg_filter, "interlace-mode", NULL, NULL, 0, "progresive",  	G_TYPE_STRING);
		}
		if( g_strv_contains(resolution, "1080p") ){
			set_filter_field(mpeg_filter, "width", 			NULL, NULL, 0, "1920",		  	G_TYPE_INT );
			set_filter_field(mpeg_filter, "height", 		NULL, NULL, 0, "1080",		  	G_TYPE_INT );
			set_filter_field(mpeg_filter, "interlace-mode", NULL, NULL, 0, "progresive",  	G_TYPE_STRING);
		}
		if( g_strv_contains(resolution, "1080i") ){
			set_filter_field(mpeg_filter, "width", 			NULL, NULL, 0, "1920",		  	G_TYPE_INT );
			set_filter_field(mpeg_filter, "height", 		NULL, NULL, 0, "1080",		  	G_TYPE_INT );
			set_filter_field(mpeg_filter, "interlace-mode", NULL, NULL, 0, "interleaved", 	G_TYPE_STRING);
		}
	}
//	set_filter_field(mpeg_filter, "framerate", GST_VIDEO_FPS_RANGE);
	printf("%s\n", gst_structure_to_string (mpeg_filter) );
	return mpeg_filter;

}
 /* This function aims to filter out input videos which do not 
 * belong to the set of VIVOE's videos by creating a filter froms the caps defined by the user into the configuration file
 * returns TRUE if the video is filter out, false otherwise
 */

gboolean filter_VIVOE(GstElement* input, GstElement* output){
	GstStructure* raw_filter 	= NULL;
	GstStructure* mpeg_filter 	= NULL;
// 	GstStructure* j2k_filter 	= NULL;
	GstCaps* vivoe_filter 	= NULL; /* the final filter to use to filters out non stream video */ 
	
	/* this function does the following: 
	 * _ open configuration file
	 * _ read and build filters (RAW, MPEG, J2k) from configuration file
	 *  _close configuration file
	 * _ create full caps from the three caps return
	 * _ link filtered input --> output
	 */

	/* open configuration file */
	GKeyFile*  gkf = open_configuration_file();
	if(gkf == NULL)
		return FALSE; /* if not configuration file was found, return NULL, meaning that we will be using the defaults filter for VIVOE formats*/
	/* Check if RAW fortmat is used bu the user */
	if( vivoe_use_format(gkf, "RAW") )
		raw_filter 	= build_RAW_filter(gkf);
	if( vivoe_use_format(gkf, "MPEG-4") )
		mpeg_filter = build_MPEG4_filter(gkf);
//	if( vivoe_use_format(gkf, "JPEG2000")		
//	j2k_filter 	= build_J2K_filter(gkf);
	
	/* close configuration file */
	close_configuration_file(gkf);

	/* create vivoe-filter */
	vivoe_filter = gst_caps_new_full ( 	raw_filter,
										mpeg_filter, 
										NULL);
	/* link element and filters out non-VIVOE format */
	if ( !gst_element_link_filtered(input, output ,vivoe_filter)){
		g_print ("Failed to link one or more elements for RAW streaming!\n");
		return FALSE;
	}else
		return TRUE;		
}
