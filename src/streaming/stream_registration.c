/*
 * Licence: GPL
 * Created: Wed, 10 Feb 2016 14:36:39 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <stdio.h>
#include <stdlib.h>

/* In order to initialize the MIB */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../../include/mibParameters.h"
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/streaming/stream_registration.h"

/* 
 * This function compares two entries in the videoFormatTable
 * if those entries are equal: return TRUE, otherwise return FALSE
 */
static gboolean compare_entries(struct videoFormatTable_entry* origin, struct videoFormatTable_entry* new){
	 if( 	(( origin->videoFormatBase 				== 	new->videoFormatBase 				) &&
			( origin->videoFormatSampling 			== 	new->videoFormatSampling 			) &&
			( origin->videoFormatBitDepth 			== 	new->videoFormatBitDepth 			) &&
			( origin->videoFormatFps 				== 	new->videoFormatFps 				) &&
			( origin->videoFormatColorimetry 		== 	new->videoFormatColorimetry 		) &&
			( origin->videoFormatInterlaced 		== 	new->videoFormatInterlaced 			) &&
			( origin->videoFormatCompressionFactor 	== 	new->videoFormatCompressionFactor 	) &&
			( origin->videoFormatCompressionRate 	== 	new->videoFormatCompressionRate 	) &&
			( origin->videoFormatMaxHorzRes 		== 	new->videoFormatMaxHorzRes 			) &&
			( origin->videoFormatMaxVertRes 		== 	new->videoFormatMaxVertRes 			) 		
		   )){
		 g_printerr("VideoFormat already exists\n");
		 return TRUE;		 
	 }else
		 return FALSE;
}
/* 
 * This function tries to save a maximum of information (in the form of capabilities) 
 * of a video stream. As each new element in pipeline may be used to gathered new relevant information 
 * to initiate the MIB, we should use this function to gathered information on the video stream
 * each time we add a element to the Gstreamer pipeline. Therefor a videoFormatTable_entry should be passed in parameters.
 * This entry should be the same for a given stream, for each time we pass through this function. 
 */
void fill_entry(GstStructure* source_str_caps, struct videoFormatTable_entry *video_info){
	
	/*videoFormatBase*/
	if( gst_structure_has_field(source_str_caps, "encoding-name") && ( video_info->videoFormatBase == NULL) ){
		video_info->videoFormatBase				= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "encoding-name"));
		video_info->videoFormatBase_len 		= strlen(video_info->videoFormatBase);
		printf("got it videoFormatBase: %s, length: %ld \n",video_info->videoFormatBase, video_info->videoFormatBase_len );
	}
	/*VideoFormatSampling*/
	if( gst_structure_has_field(source_str_caps, "sampling") && ( video_info->videoFormatSampling == NULL) ){
		video_info->videoFormatSampling 		= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "sampling"));
		video_info->videoFormatSampling_len 	= strlen(video_info->videoFormatSampling);
		printf("got it VideoFormatSampling: %s, length: %ld\n",video_info->videoFormatSampling,video_info->videoFormatSampling_len  );				
	}
	/*videoFormatBitDepth*/	
	if( gst_structure_has_field(source_str_caps, "depth") && ( video_info->videoFormatBitDepth == 0)){
		video_info->videoFormatBitDepth 		=  strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "depth")), NULL , 10 ); /* the string is converted into long int, basis 10 */
		printf("got it videoFormatBitDepth: %ld\n", video_info->videoFormatBitDepth);		
	}
	/*videoFormatFps*/	
	if( gst_structure_has_field(source_str_caps, "framerate") && ( video_info->videoFormatFps == 0)){
		video_info->videoFormatFps 				= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "framerate"));
		printf("got it videoFormatFps\n");				
	}
	/*videoFormatColorimetry*/	
	if( gst_structure_has_field(source_str_caps, "colorimetry") && ( video_info->videoFormatColorimetry == NULL)){
		video_info->videoFormatColorimetry 		= (char*)g_value_dup_string (gst_structure_get_value(source_str_caps, "colorimetry"));
		video_info->videoFormatColorimetry_len 	= strlen(video_info->videoFormatColorimetry);
		printf("got it videoFormatColorimetry: %s, legnth: %ld\n",video_info->videoFormatColorimetry,video_info->videoFormatColorimetry_len  );				
	}
	/*videoFormatInterlaced*/
	if( gst_structure_has_field(source_str_caps, "interlaced") && ( video_info->videoFormatInterlaced == 0)){
		video_info->videoFormatInterlaced 		= (long) g_value_get_int(gst_structure_get_value(source_str_caps, "interlaced "));
		printf("got it videoFormatInterlaced: %ld\n", video_info->videoFormatInterlaced);				
	}	
	/*videoFormatMaxHorzRes*/
	if( gst_structure_has_field(source_str_caps, "height") && ( video_info->videoFormatMaxHorzRes == 0)){
		if (G_VALUE_HOLDS_INT(gst_structure_get_value(source_str_caps, "height")))
			video_info->videoFormatMaxHorzRes		= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "height"));
		else
		 	video_info->videoFormatMaxHorzRes		= strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "height")), NULL , 10 ); /* the string is converted into long int, basis 10 */

		printf("got it videoFormatMaxHorzRes: %ld\n", video_info->videoFormatMaxHorzRes);
	}	
	/*videoFormatMaxVertRes*/
	if( gst_structure_has_field(source_str_caps, "width") && ( video_info->videoFormatMaxVertRes == 0)){
		if (G_VALUE_HOLDS_INT(gst_structure_get_value(source_str_caps, "width")))
			video_info->videoFormatMaxVertRes		= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "width"));			
		else
			video_info->videoFormatMaxVertRes		= strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "width")), NULL , 10 ); /* the string is converted into long int, basis 10 */

		printf("got it videoFormatMaxVertRes: %ld\n", video_info->videoFormatMaxVertRes );				
	}
}

/* Fill the MIB from information the we success to extract from the pipeline */
int initialize_videoFormat(struct videoFormatTable_entry *video_info){
	/* Check if entry already exits;
	 *  _ if yes, do not add a new entry, but set it status to enable if it is not already enable
	 *  _ if no, increase viodeFormatNumber, and  add a new entry in the table
	 */
	if(videoFormatTable_head == NULL){
		/* 	it means that this will be the first video format to be set into the table
		 *	so check if videoFormatNumber equals to zero as a second security
		 *	exit if not
		 */
		if(videoFormatNumber._value.int_val != 0){
			g_printerr("Invalid videoFormatNumber in MIB\n");
			g_printerr("No entry found in %s, so %s should be 0","videoFormatTable",videoFormatNumber._name );
			return EXIT_FAILURE;
		}else{
			/* Then we are sure that we can create a new entry */
			videoFormatTable_createEntry( 	0, 			 								videoChannel,
											enable, 									video_info->videoFormatBase,
											video_info->videoFormatBase_len,			video_info->videoFormatSampling,
											video_info->videoFormatSampling_len,		video_info->videoFormatBitDepth,
											video_info->videoFormatFps,				 	video_info->videoFormatColorimetry,
											video_info->videoFormatColorimetry_len, 	video_info->videoFormatInterlaced,
											video_info->videoFormatCompressionFactor, 	video_info->videoFormatCompressionRate, 	
											video_info->videoFormatMaxHorzRes,			video_info->videoFormatMaxVertRes, 
											0,											0, 
											0,											0,	
											0, 											0, 	
											0);

			/* increase videoFormatNumber as we added an entry */
			videoFormatNumber._value.int_val++;
		}
	}else{
		/* if the table of video format is not empty for this device, check if this format is already in the table
		 * for that, iterate the table
		 */ 
		struct videoFormatTable_entry *iterator = videoFormatTable_head ;
		video_info->videoFormatIndex = videoFormatNumber._value.int_val +1 ;
		gboolean exists = FALSE; /* a boolean to indicate weather the entry already exists in the table or not*/
		while(iterator != NULL && !exists ){
			/* compare streaming entry format to iterator format*/
			if(compare_entries(iterator,video_info)){
				/* if their the same: set the status to enable */
				iterator->videoFormatStatus = enable;
				exists = TRUE;
			}
			/* iterator = next entry */
			iterator = iterator->next;
		}
		/* check if the entry does not already exist in the table*/
		if ( !exists){
			/* So add it!
			 */
			videoFormatTable_createEntry( 	video_info->videoFormatIndex,				videoChannel,
											enable, 									video_info->videoFormatBase,
											video_info->videoFormatBase_len,			video_info->videoFormatSampling,
											video_info->videoFormatSampling_len,		video_info->videoFormatBitDepth,
											video_info->videoFormatFps,				 	video_info->videoFormatColorimetry,
											video_info->videoFormatColorimetry_len, 	video_info->videoFormatInterlaced,
											video_info->videoFormatCompressionFactor, 	video_info->videoFormatCompressionRate, 	
											video_info->videoFormatMaxHorzRes,			video_info->videoFormatMaxVertRes, 
											0,											0, 
											0,											0,	
											0, 											0, 	
											0);
		}
	/* Do not free iterator, it was positioning on HEAD */
	/* No malloc was done anyway */
	}
	return EXIT_SUCCESS;
}

