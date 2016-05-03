/*
 * Licence: GPL
 * Created: Wed, 20 Apr 2016 18:36:21 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */


#include <glib-2.0/glib.h>

#include <gstreamer-1.0/gst/gst.h>
#include <gst/video/video.h>
#include <gstreamer-1.0/gst/app/gstappsrc.h>
#include <gstreamer-1.0/gst/app/gstappsink.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "../../include/mibParameters.h"
#include "../../include/log.h"
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/channelControl/channelTable.h"
#include "../../include/mibParameters.h"
#include "../../include/conf/mib-conf.h"
#include "../../include/streaming/detect.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/streaming/stream.h"

static void set_roi_values_videoFormat_entry( struct videoFormatTable_entry *video_stream_info ,  roi_data *roi_datas) {
	
	/* set resolution */
	if ( roi_datas->roi_width || roi_datas->roi_height ){
		video_stream_info->videoFormatRoiVertRes 		= roi_datas->roi_width;
		video_stream_info->videoFormatRoiHorzRes 		= roi_datas->roi_height;
	}else{
		video_stream_info->videoFormatRoiVertRes 		= video_stream_info->videoFormatMaxVertRes ;
		video_stream_info->videoFormatRoiHorzRes 		= video_stream_info->videoFormatMaxHorzRes ;
	}

	/* set values other values */
	video_stream_info->videoFormatRoiOriginTop 		= roi_datas->roi_top;
	video_stream_info->videoFormatRoiOriginLeft 	= roi_datas->roi_left;
	video_stream_info->videoFormatRoiExtentBottom 	= roi_datas->roi_extent_bottom;
	video_stream_info->videoFormatRoiExtentRight 	= roi_datas->roi_extent_right;

	/* set the videoFormat type to roi */
	video_stream_info->videoFormatType 				= roi ;

}


/**
 * \brief specify if a SP is a roi (check if it is in the roi_table array)
 * \param videoFormatIndex the index of the videoFormat to check 
 * \return the corresponding roi_data if found or NULL 
 */
roi_data *SP_is_roi(long videoFormatIndex){

	int i = 0;
	for ( i = 0;  i< roi_table.size; i ++ ){	
		if ( roi_table.roi_datas[i]->video_SP_index == videoFormatIndex ) /* if found, then returns */
			return roi_table.roi_datas[i];
	}

	return NULL; /* if not found, returns NULL */

}

/**
 * \brief update the "config" propoerty stored in rtp_data of the given stream_data using a typefind
 * \param the steam_data we will use to detect caps and we will update
 * \param the corresponding videoformat_entry to fill
 * \return  TRUE on succes, FALSE on failure ( as caps not found for example )
 */
static gboolean SP_roi_mp4_config_update (gpointer stream_datas,  struct videoFormatTable_entry * videoFormat_entry ) {
	
	/* get the stream_data from the channel */
	stream_data 	*data 			= stream_datas; 

	/* A structure to save the new detected caps */
	GstStructure 	*video_caps;

	/*
	 * get the parser from the MPEG-4 pipeline and rtppayloader element
	 * */
	GstElement *rtpmp4vpay = gst_bin_get_by_name( GST_BIN ( data->pipeline ), "rtpmp4vpay") ;

	if ( !rtpmp4vpay ){
		g_printerr ("Failed to adapt MPEG-4 pipeline for ROI\n");
		return FALSE;
	}

	/*
	 * detect the new video caps 
	 */
 	video_caps = type_detection_for_roi( GST_BIN(data->pipeline), rtpmp4vpay , data->udp_elem ) ;
	
	if ( !video_caps ){
		g_printerr ("Failed to adapt MPEG-4 pipeline for ROI\n");
		return FALSE;
	}

	/*
	 * fill rtp data and other mib parameters in needed 
	 */
	fill_entry(video_caps, videoFormat_entry , stream_datas);

	return TRUE;

}

/**
 * \brief retrieve the scaling element used in gst_source pipeline
 * \param roi_datas the roi_data corresônding to this source
 * \return  the scaling element buit from description on succes, or NULL if no scaling element has been found
 */
GstElement *get_scaling_element ( roi_data *roi_datas ){

	GstElement *scaling_elt  = NULL;
	GError * error = NULL ;
	/*
	 * The right next element to vivoe-roi element should be the scaling element
	 * in case of a ROI channel 
	 */
	/*
	 * retreive first occurence of '!', the name of the scaling element is the name right 
	 * before
	 */
	gchar *remaining_pipeline = strstr ( roi_datas->gst_after_roi_elt , "!" );

	/* 
	 * in case there are no remaining pipeline, maybe the only element left is the scaling element 
	 * that should be the case with RAW scaling for example 
	 */
	if ( !remaining_pipeline ){
		/*
	 	* build a bin from this description
	 	*/
		scaling_elt = gst_parse_bin_from_description (roi_datas->gst_after_roi_elt ,
												TRUE,
												&error);
		
		gst_element_set_name ( scaling_elt ,  "scaling_element" );		
		
		if ( error != NULL){
			g_printerr("Failed to parse: %s\n",error->message);
	   		return NULL;	
		}

	}
	else {

		/*
		 * start by remainin the "!" from the string to parse
		 */
		remaining_pipeline += 1 ;

		gchar *name_scaling_elt		= g_strndup (roi_datas->gst_after_roi_elt , strlen( roi_datas->gst_after_roi_elt ) - strlen( remaining_pipeline ) - strlen( "! " ) );

		/* 
		 * modify the roi_data to replace gst_after_roi_elt with the remaining pipeline 
		 */

		roi_datas->gst_after_roi_elt = g_strdup ( remaining_pipeline );

		/*
		 * build a bin from this description
		 */
		scaling_elt = gst_parse_bin_from_description (name_scaling_elt ,
				TRUE,
				&error);
		
		gst_element_set_name ( scaling_elt ,  "scaling_element" );				

		if ( error != NULL){
			g_printerr("Failed to parse: %s\n",error->message);
			return NULL;	
		}

	}
	
	return scaling_elt;

}

/**
 * \brief adapt the pipeline to ROI: inset cropping element and capsfilter on the right position if needed
 * \param pipeline the corresponding pipeline to adapt
 * \param input the element to use when linking element
 * \param video_stream_info the fake videoFormat_entry to fill with new information
 * \param roi_datas the roi data corresponding to this entry
 * \param video_caps the caps of the input vidoe
 * \return input if no element added, other it returns the last element added in pipeline
 */
static GstElement *adapt_pipeline_to_roi(GstElement *pipeline , GstElement *input , struct videoFormatTable_entry *video_stream_info , roi_data *roi_datas, GstStructure *video_caps ) {

	GstElement 	*videocrop 		= NULL;
	GstElement 	*capsfilter 	= NULL;
	GstElement 	*last 			= NULL;
	GstElement *scaling_elt 	= NULL;

	/* 
	 * In any case, we will need a videocrop element in our pipeline;
	 * On the begining , start by setting its parameters to 0.
	 */

	videocrop = gst_element_factory_make_log ( "videocrop", "videocrop_roi" );
	if ( !videocrop )
		return NULL;


		g_object_set ( 	G_OBJECT ( videocrop ) , 
				"top" 		, 0, 
				"left" 		, 0, 
				"bottom" 	, 0,
				"right" 	, 0,
				NULL
				);

	last = videocrop;

	/*
	 * If the ROI is a scalble roi : the element in pipeline should be vivoe-roi scalable=true
	 * Then, 2 elements need to be inserted in pipeline: the scaling element, that we will retrieve from the gst_source
	 * command line and that should be located in roi_datas->gst_after_roi_elt. Then, once the scaling element is added in the pipeline
	 * the element that should be added in pipeline is a capsfilter. It should be set to the video resolution if the resolution of the ROI
	 * is not set in the configuration file, or to the ROI resolution given by the user in configuration file 
	 */
	if ( roi_datas->scalable ){


		/*
		 * Start by adjusting the cropping element 
		 */
		if ( 	roi_datas->roi_extent_bottom 		!= 0 	|| 
				roi_datas->roi_extent_right 		!= 0 	|| 
				roi_datas->roi_top 					!= 0 	|| 
				roi_datas->roi_left 				!= 0 ){

		g_object_set ( 	G_OBJECT ( videocrop ) , 
				"top" 		,  roi_datas->roi_top, 
				"left" 		,  roi_datas->roi_left, 
				"bottom" 	,  video_stream_info->videoFormatMaxVertRes - ( roi_datas->roi_top 	+ roi_datas->roi_extent_bottom  ),
				"right" 	,  video_stream_info->videoFormatMaxHorzRes - ( roi_datas->roi_left	+ roi_datas->roi_extent_right ),
				NULL
				);
		}

		/* but if no parameters set in configuration file, let the cropping parameters to 0 */


		scaling_elt = get_scaling_element ( roi_datas );

		/* if no scacling element found */
		if ( ! scaling_elt )
			return NULL;

		/* 
		 * For now, we consider that the user has already insert that element in its pipeline 
		 */

		capsfilter = gst_element_factory_make_log ( "capsfilter", "capsfilter_roi" );
		if ( ! capsfilter )
			return NULL;

		/* build the parameter for the caps filter */

		/* first, get the detected caps of the video  (for the meida type) */

		GstCaps *caps_filter = gst_caps_new_full ( gst_structure_copy ( video_caps ) , NULL );

		/* 
		 * check if the user have already initialized data for its ROI
		 * If so, replace height and width value with the one computed from the ROI parameter 
		 */
		if ( roi_datas->roi_width 	!= 0 	|| 
			roi_datas->roi_height 	!= 0 	 )
		{

			gst_caps_set_simple ( caps_filter , 
					"height" , G_TYPE_INT	, roi_datas->roi_height ,
					"width" , G_TYPE_INT	, roi_datas->roi_width ,
					NULL);

		}
		/*
		 * otherwise, just let the caps filter be the same resolution as the resolution found before vivoe-roi element 
		 */
		/* set the caps to the capsfilter */
		g_object_set ( 	G_OBJECT ( capsfilter ) , 
				"caps" 	, caps_filter,
				NULL
				);

	}
	/* if the ROI is not scalable */
	else{
		/* 
		 * check if the user have already initialized data for its ROI
		 * If so, replace height and width value with the one computed from the ROI parameter 
		 */
		if ( 	roi_datas->roi_width 			!= 0 	|| 
				roi_datas->roi_extent_right 	!= 0 	|| 
				roi_datas->roi_top 				!= 0 	|| 
				roi_datas->roi_left 			!= 0 ){

			g_object_set ( 	G_OBJECT ( videocrop ) , 
					"top" 		,  roi_datas->roi_top, 
					"left" 		,  roi_datas->roi_left, 
					"bottom" 	,  video_stream_info->videoFormatMaxVertRes - ( roi_datas->roi_top 	+ roi_datas->roi_height  ),
					"right" 	,  video_stream_info->videoFormatMaxHorzRes - ( roi_datas->roi_left	+ roi_datas->roi_width ),
					NULL
					);
		}

		/* if no parameters are set in configuration file, let the parameters top, left, bottom and right to 0 */
	}

	if( videocrop ){
		gst_bin_add ( GST_BIN(pipeline) , videocrop );

		if ( !gst_element_link_log (input,videocrop )){
			gst_bin_remove( GST_BIN (pipeline),videocrop );
			return NULL;
		}

		last = videocrop ;
		
		if  ( roi_datas->scalable ){
			gst_bin_add_many ( GST_BIN(pipeline) , scaling_elt , capsfilter, NULL );

			if ( !gst_element_link_many ( last , scaling_elt, capsfilter, NULL ))
				return NULL;

			last = capsfilter ;
		}

		return last;
	}
	else
		return input;

}

/**
 * \brief handle the ROI on starting: add element if needed, parse the gst_source cmdline after vivoe-roi element and add the result to pipeline
 * \param pipeline the corresponding pipeline to adapt
 * \param input the element to use when linking element
 * \param video_stream_info the fake videoFormat_entry to fill with new information
 * \param video_caps the caps of the input vidoe
 * \return input if no element added, other it returns the last element added in pipeline
 */
GstElement *handle_roi( GstElement *pipeline, GstElement *input, struct videoFormatTable_entry *video_stream_info , GstStructure *video_caps ) {

	GError 		*error 				= NULL; /* an Object to save errors when they occurs */
	GstElement *last = NULL;

	/* 
	 * Check if channel is a ROI
	 * when the structure video_stream_info has been create in file pipeline.c and function create_pipeline_videochannel 
	 * we have save the videoFormatIndex under the parameter videoFormatIndex
	 */
	roi_data *roi_datas =	SP_is_roi( video_stream_info->videoFormatIndex ) ;

	/* if this is not a roi, return input element */
	if ( !roi_datas ){
		video_stream_info->videoFormatType = videoChannel ;
		return input;
	}

	/* 
	 * if the  element vivoe-roi has been detected in pipeline, then the video is a ROI. So its type is set to ROI 
	 */
	video_stream_info->videoFormatType = roi ;

	/* 
	 * Now adapt the pipeline in consequence of the detected ROI value
	 */
	last = adapt_pipeline_to_roi ( pipeline , input , video_stream_info , roi_datas , video_caps );
	if ( !last )
		return NULL;

	/* 
	 * Set the ROI parameter into the videoFormat Entry 
	 * This will initialize the ROI parameters to 0 if they were not specified in the configuration file
	 */
	set_roi_values_videoFormat_entry( video_stream_info , roi_datas);

	/* 
	 * Now parse the rest of the gst_source pipeline
	 */
	if ( roi_datas->gst_after_roi_elt ){

		/* build last part of source pipeline */
		GstElement *bin  = gst_parse_bin_from_description ( roi_datas->gst_after_roi_elt,
												TRUE,
												&error);

		gst_element_set_name ( bin ,  "bin_after_vivoe_roi" );

		gst_bin_add ( GST_BIN(pipeline) , bin );

		if ( !gst_element_link_log (last, bin)){
			gst_bin_remove( GST_BIN (pipeline), bin);
			return NULL;
		}

		last = bin ;

	}

	return last;
		
}

/**
 * \brief update the property of videocrop and videoscale element in pipeline according to the values given by the user in the MIB
 * \param the stream_data associated to this channel
 * \param channel_entry the corresponding channel entry
 * \return TRUE on succes, FALSE on failure such as wrong values given in the MIB
 */
gboolean update_pipeline_SP_non_scalable_roi_changes( gpointer stream_datas , struct channelTable_entry *channel_entry){

	stream_data *data 		= stream_datas;
	GstElement 	*pipeline 	= data->pipeline;

	/*
	 * return if the stream data or pipeline have not been initialize 
	 */
	if ( !data || !pipeline )
		return FALSE;

	/*
	 * Get the corresponding videoFormat_Entry
	 */

	struct videoFormatTable_entry * videoFormat_entry = videoFormatTable_getEntry ( channel_entry->channelVideoFormatIndex ) ;

	/* 
	 * Get the ROi data of the videoFormat Entry
	 */
	roi_data *roi_datas =	SP_is_roi( videoFormat_entry->videoFormatIndex ) ;

	/* 
	 * The values in the table should already been updated. Here we just handle the pipeline here.
	 * We should not chceck the values entered by the user. As an example, this is not where we should 
	 * check that the ROI want from non-scalable to scalable. 
	 */

	/* we should found a crop element */
	GstElement *videocrop_roi 	= gst_bin_get_by_name ( GST_BIN ( pipeline ) , "videocrop_roi" ) ;
	GstElement *capsfilter_roi 	= gst_bin_get_by_name ( GST_BIN ( pipeline ) , "capsfilter_roi" ) ;

	/*
	 * if videocrop is not NULL, then, this is a non-scalbale ROI
	 */
	if ( videocrop_roi ){
		int top = 0 ;
		int left = 0 ;
		int bottom = 0 ;
		int right = 0 ;

		/* 
		 * If values are negative (which could happen if a bad value is set to ROI resolution, and top, then set parameters to zero
		 */
		top 	=  videoFormat_entry->videoFormatRoiOriginTop ;
		if ( top < 0 )
			top = 0 ;

		left 	=  videoFormat_entry->videoFormatRoiOriginLeft ;
		if ( left < 0 )
			left = 0 ;

		if ( roi_datas->scalable ){
			if ( videoFormat_entry->videoFormatRoiExtentBottom != 0 )
				bottom 	= videoFormat_entry->videoFormatMaxVertRes - ( videoFormat_entry->videoFormatRoiOriginTop 	+ videoFormat_entry->videoFormatRoiExtentBottom  ) ;
			if (videoFormat_entry->videoFormatRoiExtentRight != 0 )
				right 	= videoFormat_entry->videoFormatMaxHorzRes - ( videoFormat_entry->videoFormatRoiOriginLeft	+ videoFormat_entry->videoFormatRoiExtentRight  ) ;
		}else{ 
			bottom 	= videoFormat_entry->videoFormatMaxVertRes - ( videoFormat_entry->videoFormatRoiOriginTop 	+ videoFormat_entry->videoFormatRoiVertRes  ) ;
			right 	= videoFormat_entry->videoFormatMaxHorzRes - ( videoFormat_entry->videoFormatRoiOriginLeft	+ videoFormat_entry->videoFormatRoiHorzRes  ) ;			
		}

		if ( bottom < 0 )
			bottom = 0 ;

		if ( right < 0 )
			right = 0 ;

		g_object_set ( 	G_OBJECT ( videocrop_roi ) , 
				"top" 		, top , 
				"left" 		, left, 
				"bottom" 	, bottom,
				"right" 	, right,
				NULL
				);

	}
	else
	{

		g_printerr("ERROR: No ROI element has been found for this videoFormat\n");
		return FALSE;

	}
	
	/*
	 * if roi is scalable, the element capsfilter_roi should be in pipeline 
	 */
	if ( roi_datas->scalable ){
		if ( capsfilter_roi ) 
		{

			GstCaps *new_caps;
			g_object_get ( G_OBJECT( capsfilter_roi ) , "caps" , &new_caps , NULL ) ;

			new_caps = gst_caps_make_writable ( new_caps );
			gst_caps_set_simple (  new_caps , 
					"height" , G_TYPE_INT	,  videoFormat_entry->videoFormatRoiVertRes 	,
					"width" , G_TYPE_INT	,  videoFormat_entry->videoFormatRoiHorzRes 	,
					NULL);

			/* set the caps to the capsfilter */
			g_object_set ( 	G_OBJECT ( capsfilter_roi ) , 
					"caps" 	, new_caps ,
					NULL
					);

		}
		else{

			g_printerr("ERROR: No ROI element has been found for this videoFormat\n");
			return FALSE;

		}
	}


	/*
	 * Now, for MPEG-4 video only, we must update the "config" parameters for the new SDP file 
	 * that will be built 
	 */
	if ( ! strcmp( channel_entry->channelVideoFormat , MPEG4_NAME ) ){

		/*
		 * call handle mpeg config update
		 */
		if ( !SP_roi_mp4_config_update (stream_datas, videoFormat_entry ))
			return FALSE;

	}

	return TRUE;
}
