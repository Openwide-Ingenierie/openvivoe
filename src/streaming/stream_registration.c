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

/* 
 * This function compares two entries in the videoFormatTable
 * if those entries are equal: return TRUE, otherwise return FALSE
 */
static gboolean compare_entries(struct videoFormatTable_entry* origin, struct videoFormatTable_entry* new){
	return (( origin->videoFormatBase 				== new->videoFormatBase 				) &&
			( origin->videoFormatSampling 			== new->videoFormatSampling 			) &&
			( origin->videoFormatBitDepth 			== new->videoFormatBitDepth 			) &&
			( origin->videoFormatFps 				== new->videoFormatFps 					) &&
			( origin->videoFormatColorimetry 		== new->videoFormatColorimetry 			) &&
			( origin->videoFormatInterlaced 		== new->videoFormatInterlaced 			) &&
			( origin->videoFormatCompressionFactor 	== new->videoFormatCompressionFactor 	) &&
			( origin->videoFormatCompressionRate 	== new->videoFormatCompressionRate 		) &&
			( origin->videoFormatMaxHorzRes 		== new->videoFormatMaxHorzRes 			) &&
			( origin->videoFormatMaxVertRes 		== new->videoFormatMaxVertRes 			) 		
		   );
}

/* Fill the MIB from information the we success to extract from the pipeline */
int initialize_videoFormat(GstStructure* source_str_caps){
	/*
	 * Get from the caps all parameters that we need,
	 * if there not in the caps, set them to null
	 * maybe that for further implementation we should 
	 * make it necessary to specify them all
	 */

	char* videoFormatBase 				= NULL;
	size_t videoFormatBase_len 			= 0;
	char* videoFormatSampling 			= NULL; /* Should be a 16 bytes string */
	size_t videoFormatSampling_len 		= 0;
	long videoFormatBitDepth 			= 0;
	long videoFormatFps 				= 0;
	char* videoFormatColorimetry 		= NULL; /* Should be a 16 bytes string */
	size_t videoFormatColorimetry_len 	= 0;
	long videoFormatInterlaced 			= 0;
	long videoFormatCompressionFactor 	= 0;
	long videoFormatCompressionRate   	= 0;
	long videoFormatMaxHorzRes 			= 0;
 	long videoFormatMaxVertRes 			= 0;

	printf("here\n");	
	/*videoFormatBase*/
	if( gst_structure_has_field(source_str_caps, "encoding-name")){
		videoFormatBase				= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "encoding-name"));
		videoFormatBase_len 		= strlen(videoFormatBase);
		printf("got it videoFormatBase: %s, length: %ld \n",videoFormatBase, videoFormatBase_len );
	}
	/*VideoFormatSampling*/
	if( gst_structure_has_field(source_str_caps, "sampling")){
		videoFormatSampling 		= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "sampling"));
		videoFormatSampling_len 	= strlen(videoFormatSampling);
		printf("got it VideoFormatSampling: %s, length: %ld\n",videoFormatSampling,videoFormatSampling_len  );				
	}
	/*videoFormatBitDepth*/	
	if( gst_structure_has_field(source_str_caps, "depth")){
		videoFormatBitDepth 		=  strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "depth")), NULL , 10 ); /* the string is converted into long int, basis 10 */
		printf("got it videoFormatBitDepth: %ld\n", videoFormatBitDepth);		
	}
	/*videoFormatFps*/	
	if( gst_structure_has_field(source_str_caps, "framerate")){
		videoFormatFps 				= *( (long*) gst_structure_get_value(source_str_caps, "framerate"));
		printf("got it videoFormatFps\n");				
	}
	/*videoFormatColorimetry*/	
	if( gst_structure_has_field(source_str_caps, "colorimetry")){
		videoFormatColorimetry 		= (char*)g_value_dup_string (gst_structure_get_value(source_str_caps, "colorimetry"));
		videoFormatColorimetry_len 	= strlen(videoFormatColorimetry);
		printf("got it videoFormatColorimetry: %s, legnth: %ld\n",videoFormatColorimetry,videoFormatColorimetry_len  );				
	}
	/*videoFormatInterlaced*/
	if( gst_structure_has_field(source_str_caps, "interlaced")){
		videoFormatInterlaced 		= *( (long*) gst_structure_get_value(source_str_caps, "interlaced "));
		printf("got it videoFormatInterlaced: %ld\n", videoFormatInterlaced);				
	}	
	/*videoFormatMaxHorzRes*/
	if( gst_structure_has_field(source_str_caps, "height")){
	 	videoFormatMaxHorzRes		= strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "height")), NULL , 10 ); /* the string is converted into long int, basis 10 */
		printf("got it videoFormatMaxHorzRes: %ld\n", videoFormatMaxHorzRes);
	}	
	/*videoFormatMaxVertRes*/
	if( gst_structure_has_field(source_str_caps, "width")){
	 	videoFormatMaxVertRes		= strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "width")), NULL , 10 ); /* the string is converted into long int, basis 10 */
		printf("got it videoFormatMaxVertRes: %ld\n",videoFormatMaxVertRes );				
	}	

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
			videoFormatTable_createEntry( 0, 			 					videoChannel,
						enable, 						videoFormatBase,
						videoFormatBase_len,			videoFormatSampling,
						videoFormatSampling_len,		videoFormatBitDepth,
						videoFormatFps,				 	videoFormatColorimetry,
						videoFormatColorimetry_len, 	videoFormatInterlaced,
						videoFormatCompressionFactor, 	videoFormatCompressionRate, 	
						videoFormatMaxHorzRes,			videoFormatMaxVertRes, 
						0,								0, 
						0,								0,	
						0, 								0, 	
						0);
		}
	}else{
		/* if the table of video format is not empty for this device, check if this format is already in the table
		 * for that, iterate the table
		 */ 
		struct videoFormatTable_entry *iterator = videoFormatTable_head ;
		struct videoFormatTable_entry *temp;
		temp = SNMP_MALLOC_TYPEDEF(struct videoFormatTable_entry);
		if (!temp)
			return EXIT_FAILURE;
		temp->videoFormatIndex 				= videoFormatNumber._value.int_val +1 ;
		temp->videoFormatBase 				= videoFormatBase;
		temp->videoFormatBase_len 			= videoFormatBase_len;
		temp->videoFormatSampling 			= videoFormatSampling;
		temp->videoFormatSampling_len 		= videoFormatSampling_len;
		temp->videoFormatBitDepth 			= videoFormatBitDepth;
		temp->videoFormatFps 				= videoFormatFps;
		temp->videoFormatColorimetry 		= videoFormatColorimetry;
		temp->videoFormatInterlaced 		= videoFormatInterlaced;
		temp->videoFormatCompressionFactor 	= videoFormatCompressionFactor;
		temp->videoFormatCompressionRate 	= videoFormatCompressionRate;
		temp->videoFormatMaxHorzRes 		= videoFormatMaxHorzRes;
		temp->videoFormatMaxVertRes 		= videoFormatMaxVertRes;
		gboolean exists = FALSE; /* a boolean to indicate weather the entry already exists in the table or not*/
		while(iterator != NULL && !exists ){
			/* compare streaming entry format to iterator format*/
			if(compare_entries(iterator,temp)){
				/* if their the same: set the status to enable */
				iterator->videoFormatStatus = enable;
				exists = TRUE;
				/* iterator = next entry */
				iterator = iterator->next;
			}
		}
		/* check if the entry does not already exist in the table*/
		if ( !exists){
			/* So add it!
			 */
			videoFormatTable_createEntry( 	videoFormatNumber._value.int_val +1 ,			videoChannel,
											enable, 						videoFormatBase,
											videoFormatBase_len,			videoFormatSampling,
											videoFormatSampling_len,		videoFormatBitDepth,
											videoFormatFps,				 	videoFormatColorimetry,
											videoFormatColorimetry_len, 	videoFormatInterlaced,
											videoFormatCompressionFactor, 	videoFormatCompressionRate, 	
											videoFormatMaxHorzRes,			videoFormatMaxVertRes, 
											0,								0, 
											0,								0,	
											0, 								0, 	
											0);
		}
	/* free temporary entry */
	/* Do not free iterator, it was positioning on HEAD */
	free(temp);
	}
	return EXIT_SUCCESS;
}

