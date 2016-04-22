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
	GstElement *last 			= NULL; 

	if ( roi_exists ( video_stream_info ) ){
		videoconvert = gst_element_factory_make_log ( "videoconvert", "videoconvert" );
		if ( !videoconvert )
			return NULL;

		gst_bin_add ( GST_BIN(pipeline) , videoconvert);

		if ( !gst_element_link_log (input, videoconvert)){
			gst_bin_remove( GST_BIN (pipeline), videoconvert);
			return NULL;
		}
		/*
	 * This will be helpfull for all kinds of ROI
	 */
	input = videoconvert;

	}else
		return input;

		if ( roi_is_non_scalable( video_stream_info ) ){

		videocrop = gst_element_factory_make_log ( "videocrop", "videocrop" );
		if ( !videocrop )
			return NULL;

		g_object_set ( 	g_object ( videocrop ) , 
						"top" 		,  video_stream_info->videoformatroiorigintop , 
						"left" 		,  video_stream_info->videoformatroioriginleft , 
						"bottom" 	,  video_stream_info->videoformatmaxvertres - ( video_stream_info->videoformatroiorigintop 	+ video_stream_info->videoformatroivertres  ),
						"right" 	,  video_stream_info->videoformatmaxhorzres - ( video_stream_info->videoformatroioriginleft 	+ video_stream_info->videoformatroihorzres  ),
						null
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
						"height" 	,  video_stream_info->videoFormatRoiExtentBottom 	- video_stream_info->videoFormatRoiOriginTop ,
						"width" 	,  video_stream_info->videoFormatRoiExtentRight		- video_stream_info->videoFormatRoiOriginLeft,
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

	roi_data roi_datas;
	GstElement *last = NULL;

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

gboolean update_pipeline_SP_on_roi_changes( GstElement *pipeline, struct channelTable_entry *channel_entry){
	
	GstElement *source;
	GstElement *next_elem;
	/* 
	 * First checks if the channel was already an ROI.
	 * Then we just need to update the element's parameter of the ROI
	 */
	if ( channel_entry->channelType == roi ){
		/*
		 * In that case it means that the videocrop or videoscale element are already in pipeline. 
		 * the VIVOE standard also specifies that if a ROI is non-scalable it should remain as so for ever. 
		 * So no need to remove videocrop to add a videoscale element.
		 */
		
		/*
		 * Get the corresponding videoFormat_Entry
		 */
		struct videoFormatTable_entry * videoFormat_entry = videoFormatTable_getEntry ( channel_entry->channelVideoFormatIndex ) ;

		/* 
		 * The values in the table should already been updated. Here we just handle the pipeline here.
		 * We should not chceck the values entered by the user. As an example, this is not where we should 
		 * check that the ROI want from non-scalable to scalable. 
		 */
		if ( roi_is_non_scalable (  videoFormat_entry ) ){

			/* we should found a crop element */
			GstElement *videocrop = gst_bin_get_by_name ( GST_BIN ( pipeline ) , "vidocrop" ) ;
			g_object_set ( 	G_OBJECT ( videocrop ) , 
						"top" 		,  video_stream_info->videoFormatRoiOriginTop , 
						"left" 		,  video_stream_info->videoFormatRoiOriginLeft , 
						"bottom" 	,  video_stream_info->videoFormatMaxVertRes - ( video_stream_info->videoFormatRoiOriginTop 	+ video_stream_info->videoFormatRoiVertRes  ),
						"right" 	,  video_stream_info->videoFormatMaxHorzRes - ( video_stream_info->videoFormatRoiOriginLeft	+ video_stream_info->videoFormatRoiHorzRes  ),
						NULL
					);

		}else{
			return FALSE;
		}
		
		return TRUE;	

	}
	/* 
	 * Otherwise, the channel was not a ROI, this means that we need to insert a new element into the pipeline 
	 * But in order to do so, we need to be sure that all four parameters have been set before. This shouls be check in 
	 * channelTable.c
	 */
	else{

		/* first updates the videoFormatType */
		source = gst_bin_get_by_name ( GST_bin ( pipeline ) , "source" ) ;
		
		/* then get the payloader */
		if ( strcmp ( channel_entry->channelVideoFormat , RAW_NAME ) ){
			
			/* next element after source should be rtp raw payloader */
			next_elem = gst_bin_get_by_name ( GST_bin ( pipeline ) , "rtpvrawpay" ) ;
			
			/* now , pause the pipeline if it was running */
			/* save the state of pipeline */
			int state = GST_STATE( pipeline );
			if ( state == GST_STATE_PLAYING )
				gst_element_set_state (data->pipeline, GST_STATE_NULL);
			
			/* unlink source and next_elem */
			gst_element_unlink ( source , next_elem ) ;

			/* insert video crop into the pipeline */
			gst_bin_add ( GST_BIN ( pipeline ) , videocrop );
			if ( ! gst_element_link_many( source, videocrop, next_elem , NULL )){
				g_printerr ( "ERROR in gstreamer's pipeline\n");
				return FALSE;
			}

			/* of the pipeline was playing, restart it */

			if ( state  == GST_STATE_PLAYING )
				gst_element_set_state (data->pipeline, GST_STATE_PLAYING);

			return TRUE;

		}
		else if ( strcmp ( channel_entry->channelVideoFormat , MPEG4_NAME ) ){
			/* next element after source should be mpeg4 parser */
			next_elem = gst_bin_get_by_name ( GST_bin ( pipeline ) , "mpeg4videoparse" ) ;
			
			/* now , pause the pipeline if it was running */
			/* save the state of pipeline */
			int state = GST_STATE( pipeline );
			if ( state == GST_STATE_PLAYING )
				gst_element_set_state (data->pipeline, GST_STATE_NULL);
			
			/* unlink source and next_elem */
			gst_element_unlink ( source , next_elem ) ;

			/* 
			 * Cropping the video shouldbe perform with only RAW video. We need to decode the video, crop it, 
			 * encode it again.
			 */
		 	GstElement *mpeg4_decoder = gst_element_factory_make_log ( "avdec_mpeg4", "mpeg4-decoder-roi" );
			if ( !mpeg4_decoder )
				return FALSE;

		 	GstElement *mpeg4_encoder = gst_element_factory_make_log ( "avenc_mpeg4", "mpeg4-encoder-roi" );
			if ( !mpeg4_encoder)
				return FALSE;
			
			/* insert video crop into the pipeline */
			gst_bin_add_many ( GST_BIN ( pipeline ) , mpeg4_decoder , videocrop , mpeg4_encoder );
			if ( ! gst_element_link_many( source,mpeg4_decoder ,videocrop , mpeg4_encoder , next_elem , NULL )){
				g_printerr ( "ERROR in gstreamer's pipeline\n");
				return FALSE;
			}

			/* of the pipeline was playing, restart it */
			if ( state  == GST_STATE_PLAYING )
				gst_element_set_state (data->pipeline, GST_STATE_PLAYING);

			return TRUE;

		}
		else if ( strcmp ( channel_entry->channelVideoFormat , J2K_NAME ) ){
			/* next element after source should be mpeg4 parser */
			next_elem = gst_bin_get_by_name ( GST_bin ( pipeline ) , "capsfilter-image/x-jpc" ) ;
			
			/* now , pause the pipeline if it was running */
			/* save the state of pipeline */
			int state = GST_STATE( pipeline );
			if ( state == GST_STATE_PLAYING )
				gst_element_set_state (data->pipeline, GST_STATE_NULL);
			
			/* unlink source and next_elem */
			gst_element_unlink ( source , next_elem ) ;

			/* 
			 * Cropping the video shouldbe perform with only RAW video. We need to decode the video, crop it, 
			 * encode it again.
			 */
		 	GstElement *jpeg2000_decoder = gst_element_factory_make_log ( "openjpegenc", "jpeg2000-decoder-roi" );
			if ( !mpeg4_decoder )
				return FALSE;

		 	GstElement *jpeg2000_encoder = gst_element_factory_make_log ( "openjpegenc", "jpeg2000-encoder-roi" );
			if ( !mpeg4_encoder)
				return FALSE;
			
			/* insert video crop into the pipeline */
			gst_bin_add_many ( GST_BIN ( pipeline ) , jpeg2000_decoder , videocrop , jpeg2000_encoder );
			if ( ! gst_element_link_many( source , jpeg2000_decoder , videocrop , jpeg2000_encoder , next_elem , NULL )){
				g_printerr ( "ERROR in gstreamer's pipeline\n");
				return FALSE;
			}

			/* of the pipeline was playing, restart it */
			if ( state  == GST_STATE_PLAYING )
				gst_element_set_state (data->pipeline, GST_STATE_PLAYING);

			return TRUE;

		}
		else {
			g_printerr ( "ERROR: unknow video format\n" ) ;
			return FALSE;
		}
	
	}
}
