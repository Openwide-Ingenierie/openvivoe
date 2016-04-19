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
#include "../../include/streaming/detect.h"
#include "../../include/log.h"

/*
 * This is a strcuture we need to define in order to get the media type of input source video
 * */
typedef struct{
	GMainLoop 	*loop; /* the GMainLoop to use in the media type detection function */
	GstCaps 	*caps; /* the media type that will be detected */
}data_type_detection;

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
 * \param data the data needed (loop to qui, and a string to save the caps found 
 */
static void cb_typefound ( 	GstElement  			*typefind,
						    guint       			probability,
							GstCaps    				*caps,
					    	gpointer				data 
							)
{
	data_type_detection *data_type 	= data;
	GMainLoop *loop 				= data_type->loop;
	data_type->caps 				= gst_caps_copy(caps);
	/* since we connect to a signal in the pipeline thread context, we need
	 * to set an idle handler to exit the main loop in the mainloop context.
	 * Normally, your app should not need to worry about such things. */
	g_idle_add (idle_exit_loop, loop);
}

/**
 * \brief detect caps of stream using typedind module and the given sink 
 * \param pipeline the pipeline used by the stream
 * \param input_video the video stream we want to know the caps from
 * \param loop the GMainLoop the run
 * \return a pointer to the detected structure, NULL otherwise
 */
static GstStructure* type_detection_with_sink(GstBin *pipeline, GstElement *input_video, GMainLoop *loop, GstElement *sink ){
	GstElement *typefind;
	data_type_detection data;
	data.loop 	= loop;
	/* Create typefind element */
	typefind = gst_element_factory_make_log ("typefind", NULL);	

	if(typefind == NULL)
		return NULL;
	
	/* Connect typefind element to handler */
	g_signal_connect (typefind, "have-type", G_CALLBACK (cb_typefound), &data);
	gst_bin_add_many (GST_BIN (pipeline), typefind, sink, NULL);
	gst_element_link_many (input_video, typefind, sink, NULL);
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

	/* run the main loop until type is found so we execute callback function */
	if( g_main_loop_is_running (loop) ){
  		g_main_loop_quit (loop);
		was_running = TRUE; /* save the fact that we have quit the main loop */
	}

	/* run the main loop so the typefind can be performed */
	g_main_loop_run (loop);
	
	if( internal_error )
		return NULL;

	/* Video type found */	
  	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);

	/*Remove typefind and fakesink from pipeline */
	gst_element_unlink_many( input_video, typefind, sink, NULL);
	gst_bin_remove(GST_BIN(pipeline), typefind);

	/* Get video Caps */
	GstCaps 		*detected 		= data.caps;
	//printf("%s\n", gst_caps_to_string(detected));
	GstStructure 	*str_detected 	= gst_caps_get_structure(detected, 0);

	return str_detected;
}

/**
 * \brief Media Stream Capabilities detection - detect caps of stream
 * \param pipeline the pipeline used by the stream
 * \param input_video the video stream we want to know the caps from
 * \param loop the GMainLoop the run
 * \return a pointer to the detected structure, NULL otherwise
 */
GstStructure* type_detection(GstBin *pipeline, GstElement *input_video, GMainLoop *loop, GstElement *sink){
	GstStructure *str_detected;
	if (sink == NULL){
		GstElement  *fakesink;
		fakesink = gst_element_factory_make_log ("fakesink", "sink");
		if(fakesink == NULL)
			return NULL;

		str_detected = type_detection_with_sink(pipeline, input_video, loop, fakesink);
		gst_bin_remove(GST_BIN(pipeline), fakesink);

	}else{
		str_detected = type_detection_with_sink( pipeline, input_video, loop, sink);
	}
	return str_detected;
}
