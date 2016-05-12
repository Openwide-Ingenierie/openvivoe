/*
 * Licence: GPL
 * Created: Wed, 20 Apr 2016 18:36:21 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */


#include <glib-2.0/glib.h>

#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/video/video.h>
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
#include "../../include/conf/mib-conf.h"
#include "../../include/streaming/plugin/vivoecaps/vivoecaps.h"
#include "../../include/streaming/plugin/vivoecrop/gstvivoecrop.h"
#include "../../include/streaming/name.h"
#include "../../include/streaming/detect.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/streaming/stream.h"

static gboolean set_roi_values_videoFormat_entry( struct videoFormatTable_entry* videoFormat_entry , gboolean scalable ){

	long 		roi_width;
	long 		roi_height;
	long 		roi_top;
	long 		roi_left;
	long 		roi_extent_bottom;
	long 		roi_extent_right;

	if ( get_roi_parameters_for_sources ( 
			videoFormat_entry->videoFormatIndex , 	scalable, 
			&roi_width, 							&roi_height,
			&roi_top, 								&roi_left, 
			&roi_extent_bottom,   					&roi_extent_right ) ){

		/* if they are all set to 0 return FALSE so we don't lose time on settinf default values to vivoecrop and vivoeroi */
		if ( ! (roi_width && roi_height && roi_top && roi_extent_bottom && roi_extent_right ))
			return FALSE;

		/* copy the data in the videoFormat_entry */
		videoFormat_entry->videoFormatRoiHorzRes 		= roi_width; 
		videoFormat_entry->videoFormatRoiVertRes 		= roi_height; 
		videoFormat_entry->videoFormatRoiOriginTop 		= roi_top; 
		videoFormat_entry->videoFormatRoiOriginLeft 	= roi_left; 
		videoFormat_entry->videoFormatRoiExtentBottom 	= roi_extent_bottom; 
		videoFormat_entry->videoFormatRoiExtentRight 	= roi_extent_right; 

		return TRUE;

	}
	else
		return FALSE;

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
	 * detect the new video caps 
	 */

 	video_caps = type_detection_for_roi( GST_BIN(data->pipeline) , data->udp_elem ) ;
	
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
 * \brief retrieve the element of type type in a bin
 * \param pipeline the pipeline in which we are looking for vivoecrop element
 * \param type the element's type we are looking for ( should normally be GST_TYPE_VIVOE_CROP or GST_TYPE_VIVOE_CAPS )
 */
GstElement *get_element_from_bin( GstElement *pipeline  , GType type){

	GstIterator *iter = NULL;
	gboolean done;

	iter = gst_bin_iterate_recurse (GST_BIN ( pipeline ));

	done = FALSE;
	while (!done)
	{

		GValue item = G_VALUE_INIT;

		switch (gst_iterator_next (iter, &item)) {
			case GST_ITERATOR_OK:
				if (G_TYPE_CHECK_INSTANCE_TYPE(g_value_get_object (&item), type))
					return GST_ELEMENT(g_value_get_object (&item));
				else
					break;
			case GST_ITERATOR_RESYNC:
				// We don't rollback anything, we just ignore already processed ones
				gst_iterator_resync (iter);
				break;
			case GST_ITERATOR_ERROR:
				g_printerr("Error while iteratin through gst_source bin\n");
				done = TRUE;
				break;
			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}

	gst_iterator_free (iter);

	return NULL;

}

/**
 * \brief handle the ROI on starting: add element if needed, parse the gst_source cmdline after vivoe-roi element and add the result to pipeline
 * \param pipeline the corresponding pipeline to adapt
 * \param input the element to use when linking element
 * \param video_stream_info the fake videoFormat_entry to fill with new information
 * \param video_caps the caps of the input vidoe
 * \return input if no element added, other it returns the last element added in pipeline
 */
gboolean handle_roi( GstElement *pipeline, struct videoFormatTable_entry *video_stream_info , GstStructure *video_caps ) {

	GstElement 	*vivoecrop 	= NULL;
	GstElement 	*vivoecaps 	= NULL;
	gboolean scalable  		= FALSE ;

	/* 
	 * iterates though the bin source to find the element vivoecrop
	 */
	vivoecrop = get_element_from_bin( pipeline, GST_TYPE_VIVOE_CROP );	
	if ( !vivoecrop ){
		/*
		 * then this is not a ROI channel , set the type to videoChannel and exit function */
		video_stream_info->videoFormatType = videoChannel ;
		return FALSE;
	}

	/* 
	 * If we get here, it means that the element vivoecrop has been found in the pipeline, so this videoFormat is a roi
	 */
	video_stream_info->videoFormatType = roi;
	
	/* 
	 * Check if roi is scalable
	 */
	/* 
	 * iterates though the bin source to find the element vivoecaps
	 */
	vivoecaps = get_element_from_bin( pipeline, GST_TYPE_VIVOE_CAPS );
	if ( vivoecaps ) 
		scalable = TRUE ;
	/*
	 * then the ROI is scalable
	 */

	/* 
	 * Set the ROI parameter into the videoFormat Entry 
	 * This will initialize the ROI parameters to 0 if they were not specified in the configuration file
	 */
	if (set_roi_values_videoFormat_entry( video_stream_info, scalable ) ){
		
		/* Set the parameters to crop element */
		gst_vivoe_crop_update (G_OBJECT ( vivoecrop ), video_stream_info , scalable );		

		/* Set parameters to roi element */
		GstCaps *new_caps;
		g_object_get ( G_OBJECT( vivoecaps  ) , "caps" , &new_caps , NULL ) ;


		new_caps = gst_caps_make_writable ( new_caps );

		gst_caps_set_simple (  new_caps , 
				"height" , G_TYPE_INT	,  video_stream_info->videoFormatRoiVertRes 	,
				"width" , G_TYPE_INT	,  video_stream_info->videoFormatRoiHorzRes 	,
				NULL);

		/* set the caps to the capsfilter */
		g_object_set ( 	G_OBJECT ( vivoecaps ) , 
				"caps" 	, new_caps ,
				NULL
				);

	}


	return TRUE;

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
	gboolean 	scalable 	= FALSE;

	/*
	 * return if the stream data or pipeline have not been initialize 
	 */
	if ( !data || !pipeline )
		return FALSE;

	/* 
	 * The values in the table should already been updated. Here we just handle the pipeline here.
	 * We should not chceck the values entered by the user. As an example, this is not where we should 
	 * check that the ROI want from non-scalable to scalable. 
	 */
	GstElement *vivoecrop = get_element_from_bin( pipeline, GST_TYPE_VIVOE_CROP );
	GstElement *vivoecaps = get_element_from_bin( pipeline, GST_TYPE_VIVOE_CAPS );	

	if ( !vivoecrop)
		return FALSE;

	if ( vivoecaps )
		scalable = TRUE ;

	/* get the videoFormat_entry corresponding to our index */
	struct videoFormatTable_entry *videoFormat_entry = videoFormatTable_getEntry( channel_entry->channelVideoFormatIndex ) ;

	gst_vivoe_crop_update (G_OBJECT ( vivoecrop ), videoFormat_entry , scalable );

	if ( scalable )
	{
		GstCaps *new_caps;
		g_object_get ( G_OBJECT( vivoecaps  ) , "caps" , &new_caps , NULL ) ;


		new_caps = gst_caps_make_writable ( new_caps );
	
		gst_caps_set_simple (  new_caps , 
				"height" , G_TYPE_INT	,  videoFormat_entry->videoFormatRoiVertRes 	,
				"width" , G_TYPE_INT	,  videoFormat_entry->videoFormatRoiHorzRes 	,
				NULL);

		/* set the caps to the capsfilter */
		g_object_set ( 	G_OBJECT ( vivoecaps ) , 
				"caps" 	, new_caps ,
				NULL
				);
	}


	if ( ! strcmp( channel_entry->channelVideoFormat , MPEG4_NAME ) ){
		/*
		 * call handle mpeg config update
		 */
		if ( !SP_roi_mp4_config_update (stream_datas, videoFormat_entry ))
			return FALSE;
	}

	return TRUE;

}
