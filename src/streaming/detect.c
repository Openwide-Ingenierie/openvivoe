/*
 * Licence: GPL
 * Created: Wed, 10 Feb 2016 10:05:24 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/streaming/detect.h"

/*
 * This is a strcuture we need to define in order to get the media type of input source video
 * */
typedef struct{
	GMainLoop 	*loop; /* the GMainLoop to use in the media type detection function */
	char 		*type; /* the media type that will be detected */
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
static void cb_typefound ( 	GstElement 				*typefind,
						    guint       			probability,
							GstCaps    				*caps,
					    	data_type_detection 	*data)
{
	GMainLoop *loop = data->loop;
	data->type = strdup( (const char*) gst_caps_to_string (caps));
	/* since we connect to a signal in the pipeline thread context, we need
	 * to set an idle handler to exit the main loop in the mainloop context.
	 * Normally, your app should not need to worry about such things. */
	g_idle_add (idle_exit_loop, loop);
}

/**
 * \brief Media Stream Capabilities detection - detect caps of stream
 * \param pipeline the pipeline used by the stream
 * \param input_video the video stream we want to know the caps from
 * \param loop the GMainLoop the run
 */
GstStructure* type_detection(GstBin *pipeline, GstElement *input_video, GMainLoop *loop){
	GstElement  *typefind,  *fakesink;
	data_type_detection* data = malloc(sizeof(data_type_detection));
	data->loop 	= loop;
	/* Create typefind element */
	typefind = gst_element_factory_make ("typefind", "typefinder");	
	if(typefind == NULL){
		g_printerr ("Fail to detect Media Stream type\n");
		return NULL;		
	}
	/* Connect typefind element to handler */
	g_signal_connect (typefind, "have-type", G_CALLBACK (cb_typefound), data);	
  	fakesink = gst_element_factory_make ("fakesink", "sink");
	if(fakesink == NULL){
		g_printerr ("Fail to detect Media Stream type\n");
		return NULL;	
	}
  	gst_bin_add_many (GST_BIN (pipeline), typefind, fakesink, NULL);
  	gst_element_link_many (input_video, typefind, fakesink, NULL);
    gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

	/* run the main loop until type is found so we execute callback function */
	g_main_loop_run (loop);
	/* Video type found */	
  	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);

	/*Remove typefind and fakesink from pipeline */
	gst_bin_remove(GST_BIN(pipeline), typefind);
	gst_bin_remove(GST_BIN(pipeline), fakesink);
	/* Create the VIVOE pipeline for MPEG-4 videos */
	GstCaps 		*detected 		= gst_caps_from_string(data->type);
	GstStructure 	*str_detected 	= gst_caps_get_structure(detected, 0);

	free(data);
	return str_detected;
}
