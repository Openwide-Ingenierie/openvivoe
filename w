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
#include "../../include/streaming/stream_registration.h"
#include "../../include/streaming/stream.h"

static void set_roi_values_videoFormat_entry( struct videoFormatTable_entry *video_stream_info ,  roi_data *roi_datas) {
	
	/* set resolution */
	video_stream_info->videoFormatRoiVertRes 		= roi_datas->roi_width;
	video_stream_info->videoFormatRoiHorzRes 		= roi_datas->roi_height;

	/* set values other values */
	video_stream_info->videoFormatRoiOriginTop 		= roi_datas->roi_top;
	video_stream_info->videoFormatRoiOriginLeft 	= roi_datas->roi_left;
	video_stream_info->videoFormatRoiExtentBottom 	= roi_datas->roi_extent_bottom;
	video_stream_info->videoFormatRoiExtentRight 	= roi_datas->roi_extent_right;

	/* set the videoFormat type to roi */
	video_stream_info->videoFormatType 				= roi ;

}


#if 0
static gboolean roi_exists ( struct videoFormatTable_entry *video_stream_info ){
	return  ! ( video_stream_info->videoFormatRoiHorzRes 	== 0 &&
				video_stream_info->videoFormatRoiVertRes 	== 0 &&
			   	video_stream_info->videoFormatRoiOriginTop 	== 0 &&
				video_stream_info->videoFormatRoiOriginLeft == 0 );
}



static gboolean roi_is_decimated( struct videoFormatTable_entry *video_stream_info ) {

	return ( roi_exists( video_stream_info ) 																													&&
			!roi_is_non_scalable( video_stream_info ) 																											&&
		   	( video_stream_info->videoFormatRoiHorzRes < ( video_stream_info->videoFormatRoiExtentRight		- video_stream_info->videoFormatRoiOriginLeft )) 	&&
			( video_stream_info->videoFormatRoiVertRes < ( video_stream_info->videoFormatRoiExtentBottom	- video_stream_info->videoFormatRoiOriginTop )) );

}

static gboolean roi_is_interpolated( struct videoFormatTable_entry *video_stream_info ) {

	return ( roi_exists( video_stream_info ) 																													&&
			!roi_is_non_scalable( video_stream_info ) 																											&&
		   	( video_stream_info->videoFormatRoiHorzRes > ( video_stream_info->videoFormatRoiExtentRight		- video_stream_info->videoFormatRoiOriginLeft )) 	&&
			( video_stream_info->videoFormatRoiVertRes > ( video_stream_info->videoFormatRoiExtentBottom	- video_stream_info->videoFormatRoiOriginTop )) );


}
#endif // if 0

/**
 * \brief specify if a SP is a roi (check if it is in the roi_table array)
 * \param videoFormatIndex the index of the videoFormat to check 
 * \return the corresponding roi_data if found or NULL 
 */
static roi_data *SP_is_roi(long videoFormatIndex){

	int i = 0;
	for ( i = 0;  i< roi_table.size; i ++ ){	
		if ( roi_table.roi_datas[i]->video_SP_index == videoFormatIndex ) /* if found, then returns */
			return roi_table.roi_datas[i];
	}

	return NULL; /* if not found, returns NULL */

}



static GstElement *adapt_pipeline_to_roi(GstElement *pipeline , GstElement *input , struct videoFormatTable_entry *video_stream_info , roi_data *roi_datas, GstStructure *video_caps ) {

	GstElement 	*videocrop 		= NULL;
	GstElement 	*capsfilter 	= NULL;
	GstElement 	*last 			= NULL;


	/* 
	 * In any case, we will need a videocrop element in our pipeline 
	 */

	videocrop = gst_element_factory_make_log ( "videocrop", "videocrop" );
	if ( !videocrop )
		return NULL;

	/* 
	 * check if the user have already initialized data for its ROI
	 * If so, replace height and width value with the one computed from the ROI parameter 
	 */
	if ( roi_datas->roi_extent_bottom 	!= 0 	|| 
			roi_datas->roi_extent_right != 0 	|| 
			roi_datas->roi_top 			!= 0 	|| 
			roi_datas->roi_left 		!= 0 ){

		g_object_set ( 	G_OBJECT ( videocrop ) , 
				"top" 		,  roi_datas->roi_top, 
				"left" 		,  roi_datas->roi_left, 
				"bottom" 	,  video_stream_info->videoFormatMaxVertRes - ( roi_datas->roi_top 	+ roi_datas->roi_height  ),
				"right" 	,  video_stream_info->videoFormatMaxHorzRes - ( roi_datas->roi_left	+ roi_datas->roi_width ),
				NULL
				);
	}
	/*
	 * If not, just set the crop parameters to 0 
	 */
	else{

		g_object_set ( 	G_OBJECT ( videocrop ) , 
				"top" 		, 0, 
				"left" 		, 0, 
				"bottom" 	, 0,
				"right" 	, 0,
				NULL
				);
	}

	last = videocrop;

	/* if the ROI is scalable, the videoscale element should have been inserted in the pipeline, (it should certainly be the input element) */ 
	if ( roi_datas->scalable ){

		/* 
		 * For now, we consider that the user has already insert that element in its pipeline 
		 */

		capsfilter = gst_element_factory_make_log ( "capsfilter", "capsfilter_roi" );

		/* build the parameter for the caps filter */

		/* first, get the detected caps of the video  (for the meida type) */

		GstCaps *caps_filter = gst_caps_new_full ( gst_structure_copy ( video_caps ) , NULL );

		/* 
		 * check if the user have already initialized data for its ROI
		 * If so, replace height and width value with the one computed from the ROI parameter 
		 */
		if ( roi_datas->roi_extent_bottom 	!= 0 	|| 
				roi_datas->roi_extent_right != 0 	|| 
				roi_datas->roi_top 			!= 0 	|| 
				roi_datas->roi_left 		!= 0 )
		{
			gst_caps_set_simple ( caps_filter , 
					"height" , G_TYPE_INT	, video_stream_info->videoFormatRoiVertRes ,
					"width" , G_TYPE_INT	,  video_stream_info->videoFormatRoiHorzRes ,
					NULL);
		}

		/* set the caps to the capsfilter */
		g_object_set ( 	G_OBJECT ( capsfilter ) , 
				"caps" 	, caps_filter,
				NULL
				);

		last = capsfilter ;

	}

	if( last ){
		gst_bin_add ( GST_BIN(pipeline) , last );

		if ( !gst_element_link_log (input, last)){
			gst_bin_remove( GST_BIN (pipeline), last);
			return NULL;
		}

		return last;
	}
	else
		return input;

}

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
	 * The values in the table should already been updated. Here we just handle the pipeline here.
	 * We should not chceck the values entered by the user. As an example, this is not where we should 
	 * check that the ROI want from non-scalable to scalable. 
	 */

	/* we should found a crop element */
	GstElement *videocrop 	= gst_bin_get_by_name ( GST_BIN ( pipeline ) , "videocrop" ) ;
	GstElement *capsfilter 	= gst_bin_get_by_name ( GST_BIN ( pipeline ) , "capsfilter_roi" ) ;

	/*
	 * if videocrop is not NULL, then, this is a non-scalbale ROI
	 */
	if ( videocrop ){

		/* 
		 * If values are negative (which could happen if a bad value is set to ROI resolution, and top, then set parameters to zero
		 */
		int top 	=  videoFormat_entry->videoFormatRoiOriginTop ;
		if ( top < 0 )
			top = 0 ;
		int left 	=  videoFormat_entry->videoFormatRoiOriginLeft ;
		if ( left < 0 )
			left = 0 ;
		int bottom 	= videoFormat_entry->videoFormatMaxVertRes - ( videoFormat_entry->videoFormatRoiOriginTop 	+ videoFormat_entry->videoFormatRoiVertRes  ) ;
		if ( bottom < 0 )
			bottom = 0 ;
		int right 	= videoFormat_entry->videoFormatMaxHorzRes - ( videoFormat_entry->videoFormatRoiOriginLeft	+ videoFormat_entry->videoFormatRoiHorzRes  ) ;
		if ( right < 0 )
			right = 0 ;

		g_object_set ( 	G_OBJECT ( videocrop ) , 
				"top" 		, top , 
				"left" 		, left, 
				"bottom" 	, bottom,
				"right" 	, right,
				NULL
				);

		return TRUE;

	}
	else if ( capsfilter ) 
	{

		int height 	= 	videoFormat_entry->videoFormatRoiExtentBottom	- videoFormat_entry->videoFormatRoiOriginTop ;
		int width 	= 	videoFormat_entry->videoFormatRoiExtentRight 	- videoFormat_entry->videoFormatRoiOriginLeft ;

		GstCaps *new_caps;
		g_object_get ( G_OBJECT( capsfilter ) , "caps" , &new_caps , NULL ) ;
		
		/*
		 * If height or width is under or equal to 0, then it will not display anything
		 * But we need to return TRUE. It is the user's responsability to take caution when modifying the ROI
		 * parameters.
		 */
		if (  height <= 0 || width <= 0  )
			return TRUE;

		else{
			new_caps = gst_caps_make_writable ( new_caps );
			gst_caps_set_simple (  new_caps , 
					"height" , G_TYPE_INT	,  height 	,
					"width" , G_TYPE_INT	,  width 	,
					NULL);

		}

		/* set the caps to the capsfilter */
		g_object_set ( 	G_OBJECT ( capsfilter ) , 
				"caps" 	, new_caps ,
				NULL
				);

		return TRUE;

	}
	else{

		g_printerr("ERROR: No ROI element has been found for this videoFormat\n");
		return FALSE;

	}
}
