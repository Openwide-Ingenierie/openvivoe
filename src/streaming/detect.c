/*
 * Licence: GPL
 * Created: Wed, 10 Feb 2016 10:05:24 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/mibParameters.h"
#include "../../include/streaming/name.h"
#include "../../include/streaming/detect.h"
#include "../../include/log.h"

/**
 * \brief This is needed to exit the video type detection function
 * \param data the data for the function (the main loop)
 * \return FALSE
 */
static gboolean
idle_exit_loop (gpointer data)
{
  g_main_loop_quit ((GMainLoop *) data);
  /* once */
  return FALSE;
}

/**
 * \brief callback function to execute when caps have been found: stop running mainLoop, save the caps into a string
 * \param typefind a gstreamer element which lets us find the video caps
 * \param caps the caps found
 * \param data the data needed (loop to quit, and a string to save the caps found
 */
static void cb_typefound ( 	GstElement  			*typefind,
						    guint       			probability,
							GstCaps    				*caps,
					    	gpointer				found_caps
							)
{
	GstCaps **found_caps_var = found_caps;
	*found_caps_var			= gst_caps_copy(caps);
	/* since we connect to a signal in the pipeline thread context, we need
	 * to set an idle handler to exit the main loop in the mainloop context.
	 * Normally, your app should not need to worry about such things. */
	g_idle_add (idle_exit_loop, main_loop);

}

/**
 * \brief detect caps of stream using typedind module and the given sink
 * \param pipeline the pipeline used by the stream
 * \param input_video the video stream we want to know the caps from
 * \param loop the GMainLoop the run
 * \return a pointer to the detected structure, NULL otherwise
 */
static GstStructure* type_detection_with_sink(GstBin *pipeline, GstElement *input_video,GstElement *sink ){

	GstElement 	*typefind;
	GstCaps 	*found_caps;

	/* Create typefind element */
	typefind = gst_element_factory_make_log ("typefind", NULL);

	if(typefind == NULL)
		return NULL;

	/* Connect typefind element to handler */
	g_signal_connect (typefind, "have-type", G_CALLBACK (cb_typefound), &found_caps);
	gst_bin_add_many (GST_BIN (pipeline), typefind, sink, NULL);
	gst_element_link_many (input_video, typefind, sink, NULL);
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

	/* run the main loop until type is found so we execute callback function */
	if( g_main_loop_is_running (main_loop) ){
  		g_main_loop_quit (main_loop);
		was_running = TRUE; /* save the fact that we have quit the main loop */
	}

	/* run the main loop so the typefind can be performed */
	g_main_loop_run (main_loop);

	if( internal_error )
		return NULL;

	/* Video type found */
  	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);

	/*Remove typefind and fakesink from pipeline */
	gst_element_unlink_many( input_video, typefind, sink, NULL);
	gst_bin_remove(GST_BIN(pipeline), typefind);

	/* Get video Caps */
	GstCaps 		*detected 		= found_caps;
	GstStructure 	*str_detected 	= gst_caps_get_structure(detected, 0);

	return str_detected;
}

/**
 * \brief Media Stream Capabilities detection - detect caps of stream
 * \param pipeline the pipeline used by the stream
 * \param input_video the video stream we want to know the caps from
 * \return a pointer to the detected structure, NULL otherwise
 */
GstStructure* type_detection(GstBin *pipeline, GstElement *input_video , GstElement *sink){

	g_debug("start type detection");

	GstStructure *str_detected;
	if (sink == NULL){
		GstElement  *fakesink;
		fakesink = gst_element_factory_make_log ("fakesink", SINK_TYPEFIND_NAME );
		if(fakesink == NULL)
			return NULL;

		str_detected = type_detection_with_sink(pipeline, input_video, fakesink);
		gst_bin_remove(GST_BIN(pipeline), fakesink);

	}else{
		str_detected = type_detection_with_sink( pipeline, input_video, sink);
	}
	return str_detected;
}


/**
 * \brief create the typefind element to use to insert after tee element
 * \paramp pipeline the pipeline in which the typefindroi element should be added
 * \return the typefind element created
 */
GstElement *type_detection_element_for_roi( GstBin *pipeline ) {

	GstElement 	*typefind;

	g_debug("start type detection for ROI");

	/* Create typefind element */
	typefind = gst_element_factory_make_log ("typefind", TYPEFIND_ROI_NAME );

	if(typefind == NULL)
		return NULL;

	if ( ! gst_bin_add (pipeline, typefind)){
		g_critical("could not add %s in pipeline", TYPEFIND_ROI_NAME);
		return NULL;
	}

	//gst_element_set_state (GST_ELEMENT (typefind), GST_STATE_NULL);

	return typefind ;

}

/**
 * \brief this function just detect the caps between an element and a sink already in pipeline 
 */
GstStructure* type_detection_for_roi(GstBin *pipeline , GstElement *sink ){



	GstCaps 	*found_caps;

	/* retrieve typefind roi element for pipeline */
	GstElement *typefind_roi = gst_bin_get_by_name ( pipeline , TYPEFIND_ROI_NAME ) ;
	if ( !typefind_roi )
		return NULL;


	/* Connect typefind element to handler */
	g_signal_connect (typefind_roi, "have-type", G_CALLBACK (cb_typefound), &found_caps);
	/* run the main loop so the typefind can be performed */
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
	gst_element_link (typefind_roi, sink);
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

	/* run the main loop until type is found so we execute callback function */
	if( g_main_loop_is_running (main_loop) ){
  		g_main_loop_quit (main_loop);
		was_running = TRUE; /* save the fact that we have quit the main loop */
	}

	g_main_loop_run (main_loop);

	if( internal_error )
		return NULL;

	/* Video type found */
  	gst_element_set_state (GST_ELEMENT (typefind_roi), GST_STATE_NULL);

	/* unlink typefind and sink */
	gst_element_unlink (typefind_roi, sink);


	/* Get video Caps */
	GstCaps 		*detected 		= found_caps;
	GstStructure 	*str_detected 	= gst_caps_get_structure(detected, 0);

	return str_detected;

}
