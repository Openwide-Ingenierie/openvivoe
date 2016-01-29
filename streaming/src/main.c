/*
 * Licence: GPL
 * Created: Thu, 28 Jan 2016 16:44:12 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */


#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <stdio.h>

int main (int   argc,  char *argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;
   
  /* Initialize GStreamer */
  gst_init (&argc, &argv);
   
  /* Build the pipeline */
  pipeline = gst_parse_launch ("gst-launch-1.0 videotestsrc ! autovideosink", NULL);
   
  /* Start playing */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
   
  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);
  msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
   
  /* Free resources */
  if (msg != NULL)
    gst_message_unref (msg);
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return 0;
}
