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
#include "../../include/channelControl/channelTable.h"
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/multicast.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/streaming/stream.h"

/* 
 * This function compares two entries in the videoFormatTable
 * if those entries are equal: return TRUE, otherwise return FALSE
 */
static gboolean compare_entries(struct videoFormatTable_entry* origin, struct videoFormatTable_entry* new){
	 return(( !strcmp( origin->videoFormatBase 		, 	new->videoFormatBase 				)) 	&&
			( !strcmp(origin->videoFormatSampling 	, 	new->videoFormatSampling 			)) 	&&
			( origin->videoFormatBitDepth 			== 	new->videoFormatBitDepth 			) 	&&
			( origin->videoFormatFps 				== 	new->videoFormatFps 				) 	&&
			( !strcmp(origin->videoFormatColorimetry,  	new->videoFormatColorimetry 		)) 	&&
			( origin->videoFormatInterlaced 		== 	new->videoFormatInterlaced 			) 	&&
			( origin->videoFormatCompressionFactor 	== 	new->videoFormatCompressionFactor 	) 	&&
			( origin->videoFormatCompressionRate 	== 	new->videoFormatCompressionRate 	) 	&&
			( origin->videoFormatMaxHorzRes 		== 	new->videoFormatMaxHorzRes 			) 	&&
			( origin->videoFormatMaxVertRes 		== 	new->videoFormatMaxVertRes 			) 		
	   		);
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
	if( gst_structure_has_field(source_str_caps, "encoding-name")){
		video_info->videoFormatBase				= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "encoding-name"));
	}else{
		video_info->videoFormatBase 			= "";
	}
	/*VideoFormatSampling*/
	if( gst_structure_has_field(source_str_caps, "sampling")){
		video_info->videoFormatSampling 		= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "sampling"));
	}else{
		video_info->videoFormatSampling			= "";
	}
	/*videoFormatBitDepth*/	
	if( gst_structure_has_field(source_str_caps, "depth")){
		video_info->videoFormatBitDepth 		=  strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "depth")), NULL , 10 ); /* the string is converted into long int, basis 10 */
	}
	/*videoFormatFps*/	
	if( gst_structure_has_field(source_str_caps, "framerate")){
		int numerator 							= gst_value_get_fraction_numerator(gst_structure_get_value(source_str_caps, "framerate"));
		int denominator 						= gst_value_get_fraction_denominator(gst_structure_get_value(source_str_caps, "framerate"));
		video_info->videoFormatFps 				= (long) (numerator/denominator);
	}
	/*videoFormatColorimetry*/	
	if( gst_structure_has_field(source_str_caps, "colorimetry")){
		video_info->videoFormatColorimetry 		= (char*)g_value_dup_string (gst_structure_get_value(source_str_caps, "colorimetry"));
	}else{
		video_info->videoFormatColorimetry			= "";
	}
	/*videoFormatInterlaced*/
	if( gst_structure_has_field(source_str_caps, "interlace-mode")){
		 gchar* interlaced_mode					= g_value_dup_string(gst_structure_get_value(source_str_caps, "interlace-mode"));
		 if 	( !strcmp( (char*)interlaced_mode, "progressive")) 
		 	video_info->videoFormatInterlaced 		= vivoe_progressive;
		else if ( !strcmp( (char*)interlaced_mode, "interlaced"))
			video_info->videoFormatInterlaced 		= vivoe_interlaced;
	}else{
			video_info->videoFormatInterlaced 		= vivoe_none;
	}
	/*videoFormatMaxHorzRes*/
	if( gst_structure_has_field(source_str_caps, "height")){
		if (G_VALUE_HOLDS_INT(gst_structure_get_value(source_str_caps, "height")))
			video_info->videoFormatMaxHorzRes		= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "height"));
		else
		 	video_info->videoFormatMaxHorzRes		= strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "height")), NULL , 10 ); /* the string is converted into long int, basis 10 */
	}	
	/*videoFormatMaxVertRes*/
	if( gst_structure_has_field(source_str_caps, "width")){
		if (G_VALUE_HOLDS_INT(gst_structure_get_value(source_str_caps, "width")))
			video_info->videoFormatMaxVertRes		= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "width"));			
		else
			video_info->videoFormatMaxVertRes		= strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "width")), NULL , 10 ); /* the string is converted into long int, basis 10 */
	}
}

/**
 * \brief Fill the MIB from information the we success to extract from the pipeline
 * \param video_info an entry structure containing the parameters to create e new entry in the table
 * \param stream_datas the data associated to the stream, to save the index of the video format added
 * \param ip a location to store the IP computed in order to gives it as a parameter for updsink
 * \return EXIT_SUCCESS (0) or EXIT_FAILURE (1)
 */
int initialize_videoFormat(struct videoFormatTable_entry *video_info, gpointer stream_datas, long *ip){
	long index 			= 1;
	stream_data *data 	= stream_datas;	
	/* Check if entry already exits;
	 *  _ if yes, do not add a new entry, but set it status to enable if it is not already enable
	 *  _ if no, increase viodeFormatNumber, and  add a new entry in the table
	 */
	if(videoFormatTable_head == NULL){
		/* 	it means that this will be the first video format to be set into the table
		 *	so check if videoFormatNumber equals to zero as a second security
		 *	exit if not
		 */
		if((videoFormatNumber._value.int_val != 0)){
			g_printerr("Invalid videoFormatNumber in MIB\n");
			g_printerr("No entry found in %s, so %s should be 0\n","videoFormatTable",videoFormatNumber._name );
			return EXIT_FAILURE;
		}else{
			/* Then we are sure that we can create a new entry */
			videoFormatTable_createEntry( 	index 										, 			 					videoChannel,
											disable, 																	video_info->videoFormatBase,
											video_info->videoFormatSampling, 											video_info->videoFormatBitDepth,
											video_info->videoFormatFps,				 									video_info->videoFormatColorimetry,
											video_info->videoFormatInterlaced, 											video_info->videoFormatCompressionFactor, 
											video_info->videoFormatCompressionRate, 									video_info->videoFormatMaxHorzRes,			
											video_info->videoFormatMaxVertRes, 											0,
											0, 																			0,		
											0,																			0, 				
											0, 																			0);
			/* increase videoFormatNumber as we added an entry */
			videoFormatNumber._value.int_val++;

			/* update stream_datas by adding the videoFormatIndex that we just add into the videoFormatTable*/
			data->videoFormatIndex = index;	
			*ip 			= define_vivoe_multicast("enp2s0",video_info->videoFormatIndex);
			int default_ip 	= define_vivoe_multicast(DEFAULT_MULTICAST_IFACE,video_info->videoFormatIndex);

			/* compute IP */
			
			/* At the  same time we copy all of those parameters into video channel */
			channelTable_createEntry( 	index, 																			videoChannel,
										"channelUserDesc", 																stop,
										index, 																			video_info->videoFormatBase,
										video_info->videoFormatSampling, 												video_info->videoFormatBitDepth,
										video_info->videoFormatFps,				 										video_info->videoFormatColorimetry,
										video_info->videoFormatInterlaced, 												video_info->videoFormatCompressionFactor, 
										video_info->videoFormatCompressionRate, 										video_info->videoFormatMaxHorzRes,			
										video_info->videoFormatMaxVertRes, 												0,
										0, 																				0,		
										0,																				0, 				
										*ip/*IP*/, 																		0 /* packet delay*/,
 										0, /*SAP interval*/ 															index, /*defaultVideoFormatIndex*/
										default_ip/*default receive IP*/, 												stream_datas);
			/* increase channelNumber as we added an entry */			
			channelNumber._value.int_val++;			
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
		if ( !exists && (videoFormatNumber._value.int_val <=255) ){
			/* 
			 * So add it!
			 * But check that the maximum number of video format has not been reached already
			 */
			videoFormatTable_createEntry( 	video_info->videoFormatIndex,													videoChannel,
											disable, 																		video_info->videoFormatBase,
											video_info->videoFormatSampling, 												video_info->videoFormatBitDepth,
											video_info->videoFormatFps,				 										video_info->videoFormatColorimetry,
											video_info->videoFormatInterlaced,												video_info->videoFormatCompressionFactor, 
											video_info->videoFormatCompressionRate, 										video_info->videoFormatMaxHorzRes,
											video_info->videoFormatMaxVertRes, 												0,					
											0, 																				0,				
											0,																				0, 	
											0, 																				0);
			/* increase videoFormatNumber as we added an entry */
			videoFormatNumber._value.int_val++;
			/* update stream_datas by adding the videoFormatIndex that we just add into the videoFormatTable*/
			data->videoFormatIndex = video_info->videoFormatIndex;

					/* At the  same time we copy all of those parameters into video channel */
			channelTable_createEntry( 	video_info->videoFormatIndex, 														videoChannel,
										"channelUserDesc", 																	stop,
										video_info->videoFormatIndex, 														video_info->videoFormatBase,
										video_info->videoFormatSampling, 													video_info->videoFormatBitDepth,
										video_info->videoFormatFps,				 											video_info->videoFormatColorimetry,
										video_info->videoFormatInterlaced, 													video_info->videoFormatCompressionFactor, 
										video_info->videoFormatCompressionRate, 											video_info->videoFormatMaxHorzRes,			
										video_info->videoFormatMaxVertRes, 													0,
										0, 																					0,		 				
										0, 																					0,
										define_vivoe_multicast("lo",video_info->videoFormatIndex),/*receive Address*/ 		0 /* packet delay*/,
 										0, /*SAP interval*/ 																index, /*defaultVideoFormatIndex - 0 is taken by default*/
										define_vivoe_multicast("lo",index)/*default receive IP*/, 							stream_datas);
			/* increase channelNumber as we added an entry */			
			channelNumber._value.int_val++;
		}
	/* !!! IMPORTANT !!!
	 * Do not free iterator, it was positioning on HEAD */
	/* No malloc was done anyway it should not been deleted  */
	}
	return EXIT_SUCCESS;
}

