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

static void set_roi_values_videoFormat_entry( struct videoFormatTable_entry *video_stream_info ,  roi_data *roi_datas) {
	
	/* set resolution */
	video_stream_info->videoFormatRoiVertRes 		= roi_datas->roi_width;
	video_stream_info->videoFormatRoiHorzRes 		= roi_datas->roi_height;

	/* set values other values */
	video_stream_info->videoFormatRoiOriginTop 		= roi_datas->roi_top;
	video_stream_info->videoFormatRoiOriginLeft 	= roi_datas->roi_left;
	video_stream_info->videoFormatRoiExtentBottom 	= roi_datas->roi_extent_bottom;
	video_stream_info->videoFormatRoiExtentRight 	= roi_datas->roi_extent_right;

}


static gboolean roi_exists ( struct videoFormatTable_entry *video_stream_info ){
	return  ! ( video_stream_info->videoFormatRoiHorzRes 	== 0 &&
				video_stream_info->videoFormatRoiVertRes 	== 0 &&
			   	video_stream_info->videoFormatRoiOriginTop 	== 0 &&
				video_stream_info->videoFormatRoiOriginLeft == 0 );
}

static gboolean roi_is_non_scalable( struct videoFormatTable_entry *video_stream_info ) {

	return ( roi_exists ( video_stream_info ) && video_stream_info->videoFormatRoiExtentBottom == 0 && video_stream_info->videoFormatRoiExtentRight == 0 );

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

static GstElement *adapt_pipeline_to_roi(GstElement *pipeline, GstElement *input, struct videoFormatTable_entry *video_stream_info , GstStructure *video_caps ) {

	GstElement *videoconvert 	= NULL;
	GstElement *videocrop 		= NULL;
	GstElement *capsfilter 		= NULL;
	GstElement *videoscale 		= NULL;
/*
 * Save the value of input into last. This will allow us to return input if no RoI were specified in the configuration file, and
 * so if no element is added to the pipeline.
 */
	GstElement *last 			= input; 

	videoconvert = gst_element_factory_make_log ( "videoconvert ", "videoconvert" );
	if ( !videoconvert )
		return NULL;

	if ( !gst_element_link_log (input, videoconvert)){
		gst_bin_remove( GST_BIN (pipeline), videoconvert);
		return NULL;
	}

	/*
	 * Upadte input value. That is not a problem because the input element has been saved into "last", and will be return such as if 
	 * no element are added into pipeline
	 */
	input = videoconvert;

	if ( roi_is_non_scalable( video_stream_info ) ){

		videocrop = gst_element_factory_make_log ( "videocrop", "videocrop" );
		if ( !videocrop )
			return NULL;

		g_object_set ( 	G_OBJECT ( videocrop ) , 
						"top" 		, G_TYPE_INT, video_stream_info->videoFormatRoiOriginTop , 
						"left" 		, G_TYPE_INT, video_stream_info->videoFormatRoiOriginLeft , 
						"bottom" 	, G_TYPE_INT, video_stream_info->videoFormatMaxVertRes - ( video_stream_info->videoFormatRoiOriginTop 	+ video_stream_info->videoFormatRoiVertRes  ),
						"right" 	, G_TYPE_INT, video_stream_info->videoFormatMaxHorzRes - ( video_stream_info->videoFormatRoiOriginLeft 	+ video_stream_info->videoFormatRoiHorzRes  ),
						NULL
					);
		
		last = videocrop;

	}

	else if ( roi_is_decimated ( video_stream_info ) || roi_is_interpolated ( video_stream_info ) ){


		videoscale = gst_element_factory_make_log ( "videoscale", "videoscale" );
		if ( !videoscale )
			return NULL;

		capsfilter = gst_element_factory_make_log ( "capsfilter", "capsfilter_roi" );

		/* build the parameter for the caps filter */

		/* first, get the detected caps of the video  (for the meida type) */
		GstCaps *caps_filter = gst_caps_new_full ( video_caps , NULL );

		/* replace height and width value with the one computed from the ROI parameter */
		g_object_set ( 	G_OBJECT ( caps_filter ) , 
						"height" 	, G_TYPE_INT, video_stream_info->videoFormatRoiExtentBottom - video_stream_info->videoFormatRoiOriginTop ,
						"width" 	, G_TYPE_INT, video_stream_info->videoFormatRoiExtentRight	- video_stream_info->videoFormatRoiOriginLeft,
						NULL
					);
		
		/* set the caps to the capsfilter */
		g_object_set ( 	G_OBJECT ( capsfilter ) , 
						"caps" 	, caps_filter,
						NULL
					);
		
		gst_bin_add ( GST_BIN(pipeline) , capsfilter );

		if ( !gst_element_link_log (input, capsfilter)){
			gst_bin_remove( GST_BIN (pipeline), capsfilter);
			return NULL;
		}
		
		input = capsfilter ;

		last = videocrop;
	
	}

	gst_bin_add ( GST_BIN(pipeline) , last );

	if ( !gst_element_link_log (input, last)){
		gst_bin_remove( GST_BIN (pipeline), last);
		return NULL;
	}

	return last;

}

GstElement *handle_roi( GstElement *pipeline, GstElement *input, struct videoFormatTable_entry *video_stream_info , GstStructure *video_caps ) {

	roi_data roi_datas;
	GstElement *last;

	/* retrieve default value from configuration */
	if ( !get_roi_parameters_for_sources ( video_stream_info->videoFormatIndex , &roi_datas) )
		return NULL;

	/* 
	 * Set the ROI parameter into the videoFormat Entry 
	 * This will initialize the ROI parameters to 0 if they were not specified in the configuration file
	 */
	set_roi_values_videoFormat_entry( video_stream_info , &roi_datas);

	/* 
	 * Now adapt the pipeline in consequence of the detected ROI value
	 */
	last = adapt_pipeline_to_roi ( pipeline , input , video_stream_info , video_caps );

	return last;
		
}
