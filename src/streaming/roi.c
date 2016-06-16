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

static gboolean get_roi_values_from_conf( struct videoFormatTable_entry* videoFormat_entry , gboolean scalable , struct channelTable_entry *channel_entry){

	long 		roi_width;
	long 		roi_height;
	long 		roi_top;
	long 		roi_left;
	long 		roi_extent_bottom;
	long 		roi_extent_right;

	/*
	 * If channel_entry is  not NULL, this is a SU ROI so look for sink ROI parameters
	 */
	if ( channel_entry ){

			if ( get_roi_parameters_for_sink (
					channel_entry->channelIndex, 	scalable,
					&roi_width, 					&roi_height,
					&roi_top, 						&roi_left,
					&roi_extent_bottom,   			&roi_extent_right ) ){

			/* if they are all set to 0 return FALSE so we don't lose time on settinf default values to vivoecrop and vivoeroi */
			if ( ! (roi_width && roi_height && roi_top && roi_extent_bottom && roi_extent_right ))
				return FALSE;

			/*
			 * save those parameters into the videoFormat_entry
			 */
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

	}else{
		/*
		 * If channel_entry is NULL, we are a SP ROI, so we look into configuration file for [source_x] groups
		 */

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
		g_critical ("Failed to adapt MPEG-4 pipeline for ROI");
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
				g_critical("while iteratin through gst_source bin");
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

static gboolean
set_parameters_to_roi_elements ( struct videoFormatTable_entry *video_stream_info ,	GstElement 	*vivoecrop, GstElement 	*vivoecaps, gboolean scalable ){

	/* Set the parameters to crop element */
	gst_vivoe_crop_update (G_OBJECT ( vivoecrop ), video_stream_info , scalable );

	if ( scalable ){

		/*
		 * Set parameters to roi element
		 * As we are after a scaling element we can built a caps that is a RAW video
		 */
		GstPad *vivoecrop_src_pad 	= gst_element_get_static_pad( vivoecrop , "src");
		GstCaps *new_caps;
		if ( vivoecrop_src_pad)
			new_caps  			=	gst_caps_copy (gst_pad_get_pad_template_caps (vivoecrop_src_pad));
		else{
			g_critical("Faile to get vivoecrop source pad template");
			return FALSE;
		}

		gst_caps_set_simple (  new_caps ,
				"height" , G_TYPE_INT	,  video_stream_info->videoFormatRoiVertRes	,
				"width" , G_TYPE_INT	,  video_stream_info->videoFormatRoiHorzRes	,
				NULL);

		/* set the caps to the capsfilter */
		g_object_set ( 	G_OBJECT ( vivoecaps ) ,
				"caps" 	, new_caps ,
				NULL
				);

		GstCaps *new_caps_bis;
		g_object_get ( G_OBJECT( vivoecaps  ) , "caps" , &new_caps_bis , NULL ) ;

	}

	return TRUE ;

}

/**
 * \brief handle the ROI on starting: add element if needed, parse the gst_source cmdline after vivoe-roi element and add the result to pipeline
 * \param pipeline the corresponding pipeline to adapt
 * \param input the element to use when linking element
 * \param video_stream_info the fake videoFormat_entry to fill with new information
 * \param video_caps the caps of the input vidoe
 * \return input if no element added, other it returns the last element added in pipeline
 */
gboolean handle_roi( GstElement *pipeline, struct videoFormatTable_entry *video_stream_info , struct channelTable_entry *channel_entry ) {

	GstElement 	*vivoecrop 	= NULL;
	GstElement 	*vivoecaps 	= NULL;
	gboolean 	scalable  	= FALSE ;

	g_debug("handle_roi()");

	/*
	 * iterates though the bin source to find the element vivoecrop
	 */
	vivoecrop = get_element_from_bin( pipeline, GST_TYPE_VIVOE_CROP );
	if ( !vivoecrop ){
		g_debug("This is a not ROI channel");
		/*
		 * then this is not a ROI channel , set the type to videoChannel and exit function */
		video_stream_info->videoFormatType = videoChannel ;
		return FALSE;
	}

	g_debug("This is a ROI channel");

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
	if ( get_roi_values_from_conf( video_stream_info, scalable , channel_entry ) ){
		set_parameters_to_roi_elements ( video_stream_info , vivoecrop, vivoecaps , scalable );
		return TRUE;
	}
	else
		return TRUE;

}

/**
 * \brief update the property of videocrop and videoscale element in pipeline according to the values given by the user in the MIB
 * \param the stream_data associated to this channel
 * \param channel_entry the corresponding channel entry
 * \return TRUE on succes, FALSE on failure such as wrong values given in the MIB
 */
gboolean update_pipeline_on_roi_changes( gpointer stream_datas , struct channelTable_entry *channel_entry , struct videoFormatTable_entry *videoFormat_entry){

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
	GstElement *vivoecrop = get_element_from_bin( pipeline , GST_TYPE_VIVOE_CROP );
	GstElement *vivoecaps = get_element_from_bin( pipeline , GST_TYPE_VIVOE_CAPS );

	if ( !vivoecrop)
		return FALSE;

	if ( vivoecaps )
		scalable = TRUE ;

	set_parameters_to_roi_elements ( videoFormat_entry , vivoecrop, vivoecaps, scalable );

	if (  (! strcmp( channel_entry->channelVideoFormat , MPEG4_NAME )) && channel_entry->channelType != serviceUser ){
		/*
		 * call handle MPEG4 config update
		 */
		if ( !SP_roi_mp4_config_update (stream_datas, videoFormat_entry ))
			return FALSE;
	}

	return TRUE;

}

/**
 * \brief 	replace the argument %a %b %c %d of a string by the values of Roi origin and Extent parameters
 * \param 	camera_ctl the corresponding string in which we should find '%a' '%b' '%c' '%d'
 * \param 	channel_entry the entry from which retrieve the value of Roi origin and extent object
 * \return 	the corresponding string where %a %b %c or %d characters have been replaced by their numerical value
*/
 static gchar* camera_ctl_printf( gchar *camera_ctl , struct channelTable_entry *channel_entry ){

	/* parse the command line to find the correspondance to the channel entry parameters */
	gchar *parse = strchr( (const char*) camera_ctl , '%' );
	gchar *previous_parse = camera_ctl;
	gchar *return_string = (gchar*) malloc( sizeof(gchar) * (strlen(camera_ctl) + 50 ) );
	/*
	 * The memory allocated for the returned string should be greater than the original string and here's why:
	 * each time a %a or %b or %c or %d will be found its value will be replaced by its numerical correspondance according to
	 * the values of the channel's parameters. When this parameters worth 100 or 1000 or 1000, the characters %a will be replaced
	 * by 100, 1000, 10000. Thus this add to the string 3, 4 or 5 characters when only 2 characers are replaced: '%' and 'a'.
	 * Thus we need to allocated more memory than the original string size. 50 more bytes should be large enough. Howver, this is a quick
	 * fix, but it is not optimal. A optimal solution would be to realloc return_string each time the we print more than two characters into it
	 */
	gchar *return_string_head = return_string;
	gboolean end_string = FALSE;
	int wrote;

	while ( parse != NULL && end_string == FALSE ){
		switch ( *(parse + 1) ){
			case 'a':
				strncpy( return_string , previous_parse , strlen (previous_parse) - strlen ( parse ) );
				return_string +=  strlen ( previous_parse ) - strlen ( parse );
				wrote = sprintf(return_string , "%ld", channel_entry->channelRoiOriginTop);
				return_string += wrote;
				parse += 2;
				break;
			case 'b':
				strncpy( return_string , previous_parse , strlen (previous_parse) - strlen ( parse ) );
				return_string +=  strlen ( previous_parse ) - strlen ( parse );
				wrote = sprintf(return_string , "%ld", channel_entry->channelRoiOriginLeft);
				return_string += wrote;
				parse += 2;
				break;
			case 'c':
				strncpy( return_string , previous_parse , strlen (previous_parse) - strlen ( parse ) );
				return_string +=  strlen ( previous_parse ) - strlen ( parse );
				wrote = sprintf(return_string , "%ld", channel_entry->channelRoiExtentBottom);
				return_string += wrote;
				parse += 2;
				break;
			case 'd':
				strncpy( return_string , previous_parse , strlen (previous_parse) - strlen ( parse ) );
				return_string +=  strlen ( previous_parse ) - strlen ( parse );
				wrote = sprintf(return_string , "%ld", channel_entry->channelRoiExtentRight);
				return_string += wrote;
				parse += 2;
				break;
			default:
				strncpy(return_string,  previous_parse , strlen (previous_parse) - strlen ( parse ) + 1);
				return_string +=  strlen ( previous_parse ) - strlen ( parse ) + 1;
				parse++;
				break;
		}
		previous_parse = parse;
		parse = strchr( (const char*) parse , '%' );
	}

	/* terminate string properly */
	strncpy ( return_string, "\0", 1 );

	return return_string_head;

}


/**
 * \brief if the key camera_ctl is in configuration then ROI should configured the camera on not the gstreamer's vivoe plugins
 */
gboolean update_camera_ctl_on_roi_changes ( struct channelTable_entry *channel_entry )
{
	int ret = 0;

	/* first retrieve the command line to use to control the camera and its arguments */
	gchar 	*camera_ctl 	= get_camera_ctl_cmdline ( channel_entry->channelVideoFormatIndex ) ;

	if ( !camera_ctl ){
		return FALSE;
	}

	gchar *command_line = camera_ctl_printf( camera_ctl , channel_entry );

	/* execute command */
	ret = system(command_line);

	/* check the returned value of the command execution */
	if ( ret < 0 )
	{
		g_critical("Failed to launch a the command to controle camera of source_%ld: %s", channel_entry->channelVideoFormatIndex , strerror(errno));
		return FALSE;
	}

	/* command_line is a newly allocated string where memory have been allocated in camera_ctl_printf(), we need to free it */
	free ( command_line );

	return TRUE;

}
