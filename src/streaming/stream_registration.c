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
#include "include/mibParameters.h"
#include "include/videoFormatInfo/videoFormatTable.h"
#include "include/conf/mib-conf.h"
#include "include/channelControl/channelTable.h"
#include "include/streaming/stream_registration.h"
#include "include/streaming/stream.h"
#include "include/announcement/sap.h"

/**
 * \brief J2K videos caps have a field names "colorspace" that can be map to a classic slampling
 * \param colorimetru the colorimetry srting value in detected caps
 * \return gchar* the mapped value
 */
gchar* map_colorimetry_to_sampling_j2k(const gchar *colorspace){

	/* compare values that can be in the colorimetry field to the actual value in caps */
	/* the values that can takes the colorimetry field can be found by launching the following command
	 * "gst-inspect-1.0 openjpegenc" 
	 */
	if ( !strcmp(colorspace ,colo_RGB ))
		return sampling_RGB;
	else if ( !strcmp(colorspace ,colo_YUV ))
		return sampling_YUV;
	else if ( !strcmp(colorspace ,colo_GRAY ))
		return samping_GRAY;
	else 
		return "";
}

/**
 * \brief J2K videos caps have a field names "sampling" that can be map to a classic colorspace
 * \param sampling the sampling srting value in detected caps
 * \return gchar* the mapped value
 */
gchar* map_sampling_to_colorimetry_j2k(const gchar *sampling){

	/* compare values that can be in the colorimetry field to the actual value in caps */
	/* the values that can takes the colorimetry field can be found by launching the following command
	 * "gst-inspect-1.0 openjpegenc"
	 */
	if ( !strcmp(sampling ,sampling_RGB ))
		return colo_RGB;
	else if ( !strcmp(sampling ,sampling_YUV ))
		return colo_YUV;
	else if ( !strcmp(sampling ,samping_GRAY ))
		return colo_GRAY;
	else
		return "";

}
/*
 * This function tries to save a maximum of information (in the form of capabilities)
 * of a video stream. As each new element in pipeline may be used to gathered new relevant information
 * to initiate the MIB, we should use this function to gathered information on the video stream
 * each time we add a element to the Gstreamer pipeline. Therefor a videoFormatTable_entry should be passed in parameters.
 * This entry should be the same for a given stream, for each time we pass through this function.
 */
void fill_entry(GstStructure* source_str_caps, struct videoFormatTable_entry *video_info, gpointer stream_datas){
	stream_data *data 			= stream_datas;
	/*videoFormatBase*/
	if( gst_structure_has_field(source_str_caps, "encoding-name") ){
		video_info->videoFormatBase					= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "encoding-name"));
	}else if ( video_info->videoFormatBase==NULL ){
		video_info->videoFormatBase 				= "";
	}
	/*VideoFormatSampling*/
	/* case of RAW type video */
	if( gst_structure_has_field(source_str_caps, "sampling") ){
		video_info->videoFormatSampling 			= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "sampling"));
	}else if( video_info->videoFormatSampling==NULL ){
		video_info->videoFormatSampling				= "";
	}
	/* case of J2K type video */
	if( gst_structure_has_field(source_str_caps, "colorspace")){
		gchar* colorspace 							= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "colorspace"));
		video_info->videoFormatSampling 			= g_strdup(map_colorimetry_to_sampling_j2k(colorspace));
	}else if( video_info->videoFormatSampling==NULL ){
		video_info->videoFormatSampling				= "";
	}
	/*videoFormatBitDepth*/
	if( gst_structure_has_field(source_str_caps, "depth")){
		video_info->videoFormatBitDepth 			=  strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "depth")), NULL , 10 ); /* the string is converted into long int, basis 10 */
	}
	/*videoFormatFps*/
	if( gst_structure_has_field(source_str_caps, "framerate")){
		int numerator 								= gst_value_get_fraction_numerator(gst_structure_get_value(source_str_caps, "framerate"));
		int denominator 							= gst_value_get_fraction_denominator(gst_structure_get_value(source_str_caps, "framerate"));
		video_info->videoFormatFps 					= (long) (numerator/denominator);
	}
	/*videoFormatColorimetry*/
	if( gst_structure_has_field(source_str_caps, "colorimetry")){
		video_info->videoFormatColorimetry 			= (char*)g_value_dup_string (gst_structure_get_value(source_str_caps, "colorimetry"));
	}else if( video_info->videoFormatColorimetry==NULL ){
		video_info->videoFormatColorimetry			= "";
	}
	/*videoFormatInterlaced*/
	if( gst_structure_has_field(source_str_caps, "interlace-mode")){
		gchar* video_interlaced_mode						= g_value_dup_string(gst_structure_get_value(source_str_caps, "interlace-mode"));
		if 	( !strcmp( (char*)video_interlaced_mode, "progressive"))
			video_info->videoFormatInterlaced 		= vivoe_progressive;
		else if ( !strcmp( (char*)video_interlaced_mode, "interlaced"))
			video_info->videoFormatInterlaced 		= vivoe_interlaced;
		else
			video_info->videoFormatInterlaced 		= vivoe_none;
	}
	/*videoFormatMaxVertRes*/
	if( gst_structure_has_field(source_str_caps, "height")){
		if (G_VALUE_HOLDS_INT(gst_structure_get_value(source_str_caps, "height")))
			video_info->videoFormatMaxVertRes		= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "height"));
		else
			video_info->videoFormatMaxVertRes		= strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "height")), NULL , 10 ); /* the string is converted into long int, basis 10 */
	}
	/*videoFormatMaxHorzRes*/
	if( gst_structure_has_field(source_str_caps, "width")){
		if (G_VALUE_HOLDS_INT(gst_structure_get_value(source_str_caps, "width")))
			video_info->videoFormatMaxHorzRes		= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "width"));
		else
			video_info->videoFormatMaxHorzRes		= strtol ( (char* ) g_value_dup_string ( gst_structure_get_value(source_str_caps, "width")), NULL , 10 ); /* the string is converted into long int, basis 10 */
	}

	/*channelRTppt (saved in rtp_datas - payload type of the channel should be something like 96 or 127*/
	if( gst_structure_has_field(source_str_caps, "payload")){
		if (G_VALUE_HOLDS_INT(gst_structure_get_value(source_str_caps, "payload"))){
			data->rtp_datas->rtp_type 				= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "payload"));
			/* case of a service USer, we need to initialize Rtppt */
			video_info->videoFormatRtpPt 			= (long) g_value_get_int( gst_structure_get_value(source_str_caps, "payload"));
		}
	}

	/* clock-rate */
	if( gst_structure_has_field(source_str_caps, "clock-rate")){
		if (G_VALUE_HOLDS_INT(gst_structure_get_value(source_str_caps, "clock-rate")))
			data->rtp_datas->clock_rate				= g_value_get_int( gst_structure_get_value(source_str_caps, "clock-rate"));
	}
	/* ------------------------------------------------------------------- */
	/* ONLY FOR MPEG-4 videos - relevant information to build SDP messages */

	/* profile-level-id */
	if( gst_structure_has_field(source_str_caps, "profile-level-id")){
		data->rtp_datas->profile_level_id		= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "profile-level-id"));
	}

	/* config */
	if( gst_structure_has_field(source_str_caps, "config")){
		data->rtp_datas->config 				= (char*) g_value_dup_string (gst_structure_get_value(source_str_caps, "config"));
	}
}

/**
 * \brief Fill the MIB from information the we success to extract from the pipeline
 * \param video_info an entry structure containing the parameters to create e new entry in the table
 * \param stream_datas the data associated to the stream, to save the index of the video format added
 * \return EXIT_SUCCESS (0) or EXIT_FAILURE (1)
 */
int initialize_videoFormat(struct videoFormatTable_entry *video_info, gpointer stream_datas, long *channel_entry_index ){
	stream_data *data 			= stream_datas;

	g_debug("intialize new videoFormatEntry and new channelEntry in table");

	/* Then we are sure that we can create a new entry */
		videoFormatTable_createEntry( 	video_info->videoFormatIndex, 						video_info->videoFormatType,
				disable, 																	video_info->videoFormatBase,
				video_info->videoFormatSampling, 											video_info->videoFormatBitDepth,
				video_info->videoFormatFps,				 									video_info->videoFormatColorimetry,
				video_info->videoFormatInterlaced, 											video_info->videoFormatCompressionFactor,
				video_info->videoFormatCompressionRate, 									video_info->videoFormatMaxHorzRes,
				video_info->videoFormatMaxVertRes, 											video_info->videoFormatRoiHorzRes,
				video_info->videoFormatRoiVertRes, 											video_info->videoFormatRoiOriginTop,
				video_info->videoFormatRoiOriginLeft,										video_info->videoFormatRoiExtentBottom,
				video_info->videoFormatRoiExtentRight, 										data->rtp_datas->rtp_type,
				data);

		char *channelUserDesc = get_desc_from_conf(video_info->videoFormatIndex);
		if (channelUserDesc == NULL)
			channelUserDesc="";

		/* initialize channel resolution to roi resolution if this is a roi or to video resoltion if there is no ROI */
		int channelHorzRes;
		int channelVertRes;
		if ( video_info->videoFormatRoiHorzRes && video_info->videoFormatRoiVertRes ){

			channelHorzRes = video_info->videoFormatRoiHorzRes;
			channelVertRes = video_info->videoFormatRoiVertRes;

		}else{

			channelHorzRes = video_info->videoFormatMaxHorzRes;
			channelVertRes = video_info->videoFormatMaxVertRes;

		}

		/* At the  same time we copy all of those parameters into video channel */
		channelTable_createEntry( 	channelNumber._value.int_val+1, 							video_info->videoFormatType,
				channelUserDesc, 																channelStop,
				video_info->videoFormatIndex, 													video_info->videoFormatBase,
				video_info->videoFormatSampling, 												video_info->videoFormatBitDepth,
				video_info->videoFormatFps,				 										video_info->videoFormatColorimetry,
				video_info->videoFormatInterlaced, 												video_info->videoFormatCompressionFactor,
				video_info->videoFormatCompressionRate, 										channelHorzRes,
				channelVertRes, 																video_info->videoFormatRoiOriginTop,
				video_info->videoFormatRoiOriginLeft, 											video_info->videoFormatRoiExtentBottom,
				video_info->videoFormatRoiExtentRight,											data->rtp_datas->rtp_type,
				0/*IP*/, 																		0 /* packet delay*/,
				default_SAP_interval,															video_info->videoFormatIndex, /*defaultVideoFormatIndex*/
				0/*default receive IP*/, 														data);

		*channel_entry_index = channelNumber._value.int_val+1;
		/* increase channelNumber as we added an entry */
		channelNumber._value.int_val++;

	return EXIT_SUCCESS;

#if 0
	if(videoFormatTable_head == NULL){

		/* Then we are sure that we can create a new entry */
		videoFormatTable_createEntry( 	video_info->videoFormatIndex, 						video_info->videoFormatType,
				disable, 																	video_info->videoFormatBase,
				video_info->videoFormatSampling, 											video_info->videoFormatBitDepth,
				video_info->videoFormatFps,				 									video_info->videoFormatColorimetry,
				video_info->videoFormatInterlaced, 											video_info->videoFormatCompressionFactor,
				video_info->videoFormatCompressionRate, 									video_info->videoFormatMaxHorzRes,
				video_info->videoFormatMaxVertRes, 											video_info->videoFormatRoiHorzRes,
				video_info->videoFormatRoiVertRes, 											video_info->videoFormatRoiOriginTop,
				video_info->videoFormatRoiOriginLeft,										video_info->videoFormatRoiExtentBottom,
				video_info->videoFormatRoiExtentRight, 										data->rtp_datas->rtp_type,
				data);

		char *channelUserDesc = get_desc_from_conf(video_info->videoFormatIndex);
		if (channelUserDesc == NULL)
			channelUserDesc="";

		/* initialize channel resolution to roi resolution if this is a roi or to video resoltion if there is no ROI */
		int channelHorzRes;
		int channelVertRes;
		if ( video_info->videoFormatRoiHorzRes && video_info->videoFormatRoiVertRes ){

			channelHorzRes = video_info->videoFormatRoiHorzRes;
			channelVertRes = video_info->videoFormatRoiVertRes;

		}else{

			channelHorzRes = video_info->videoFormatMaxHorzRes;
			channelVertRes = video_info->videoFormatMaxVertRes;

		}

		/* At the  same time we copy all of those parameters into video channel */
		channelTable_createEntry( 	channelNumber._value.int_val+1, 							video_info->videoFormatType,
				channelUserDesc, 																channelStop,
				video_info->videoFormatIndex, 													video_info->videoFormatBase,
				video_info->videoFormatSampling, 												video_info->videoFormatBitDepth,
				video_info->videoFormatFps,				 										video_info->videoFormatColorimetry,
				video_info->videoFormatInterlaced, 												video_info->videoFormatCompressionFactor,
				video_info->videoFormatCompressionRate, 										channelHorzRes,
				channelVertRes, 																video_info->videoFormatRoiOriginTop,
				video_info->videoFormatRoiOriginLeft, 											video_info->videoFormatRoiExtentBottom,
				video_info->videoFormatRoiExtentRight,											data->rtp_datas->rtp_type,
				0/*IP*/, 																		0 /* packet delay*/,
				default_SAP_interval,															video_info->videoFormatIndex, /*defaultVideoFormatIndex*/
				0/*default receive IP*/, 														data);

		*channel_entry_index = channelNumber._value.int_val+1;
		/* increase channelNumber as we added an entry */
		channelNumber._value.int_val++;

	}else{
		/* if the table of video format is not empty for this device, check if this format is already in the table
		 * for that, iterate the table
		 */
		struct videoFormatTable_entry *iterator = videoFormatTable_head ;
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
			videoFormatTable_createEntry( 	video_info->videoFormatIndex,													video_info->videoFormatType,
											disable, 																		video_info->videoFormatBase,
											video_info->videoFormatSampling, 												video_info->videoFormatBitDepth,
											video_info->videoFormatFps,				 										video_info->videoFormatColorimetry,
											video_info->videoFormatInterlaced,												video_info->videoFormatCompressionFactor,
											video_info->videoFormatCompressionRate, 										video_info->videoFormatMaxHorzRes,
											video_info->videoFormatMaxVertRes, 												video_info->videoFormatRoiHorzRes,
											video_info->videoFormatRoiVertRes, 												video_info->videoFormatRoiOriginTop,
											video_info->videoFormatRoiOriginLeft,											video_info->videoFormatRoiExtentBottom,
											video_info->videoFormatRoiExtentRight, 											data->rtp_datas->rtp_type,
											data );

			/* increase videoFormatNumber as we added an entry */
			char *channelUserDesc = get_desc_from_conf(video_info->videoFormatIndex);
			if (channelUserDesc == NULL)
				channelUserDesc="";

		/* initialize channel resolution to roi resolution if this is a roi or to video resoltion if there is no ROI */
		int channelHorzRes;
		int channelVertRes;
		if ( video_info->videoFormatRoiHorzRes && video_info->videoFormatRoiVertRes ){

			channelHorzRes = video_info->videoFormatRoiHorzRes;
			channelVertRes = video_info->videoFormatRoiVertRes;

		}else{

			channelHorzRes = video_info->videoFormatMaxHorzRes;
			channelVertRes = video_info->videoFormatMaxVertRes;

		}

			/* At the  same time we copy all of those parameters into video channel */
			channelTable_createEntry( 	channelNumber._value.int_val+1, 													video_info->videoFormatType,
										channelUserDesc, 																	channelStop,
										video_info->videoFormatIndex, 														video_info->videoFormatBase,
										video_info->videoFormatSampling, 													video_info->videoFormatBitDepth,
										video_info->videoFormatFps,				 											video_info->videoFormatColorimetry,
										video_info->videoFormatInterlaced, 													video_info->videoFormatCompressionFactor,
										video_info->videoFormatCompressionRate, 											channelHorzRes,
										channelVertRes, 				 													0,
										0, 																					0,
										0,																					data->rtp_datas->rtp_type,
										0,/*receive Address*/ 																0 /* packet delay*/,
			 							default_SAP_interval,  																video_info->videoFormatIndex, /*defaultVideoFormatIndex-0 is taken by default*/
										0/*default receive IP*/, 		 													data);

			*channel_entry_index = channelNumber._value.int_val+1;
			/* increase channelNumber as we added an entry */
			channelNumber._value.int_val++;
		}
	/* !!! IMPORTANT !!!
	 * Do not free iterator, it was positioning on HEAD */
	/* No malloc was done anyway it should not been deleted  */
	}
	return EXIT_SUCCESS;
#endif

}

