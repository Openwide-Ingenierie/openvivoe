/*
 * Licence: GPL
 * Created: Thu, 28 Jan 2016 16:44:12 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */


#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <stdio.h>
#include <stdlib.h>

#define IP 127.0.0.1 /*this is the address of the server to sink the UPD datagrams*/
#define PORT 1993 /*this is the port number associated to the udp socket created for th sink*/

static gboolean bus_call (GstBus     *bus,
                          GstMessage *msg,
                          gpointer    data)
{
        GMainLoop *loop = (GMainLoop *) data;

        switch (GST_MESSAGE_TYPE (msg)) {

                case GST_MESSAGE_EOS:
                        g_print ("End of stream\n");
                        g_main_loop_quit (loop);
                        break;

                case GST_MESSAGE_ERROR: {
                                                gchar  *debug;
                                                GError *error;

                                                gst_message_parse_error (msg, &error, &debug);
                                                g_free (debug);

                                                g_printerr ("Error: %s\n", error->message);
                                                g_error_free (error);

                                                g_main_loop_quit (loop);
                                                break;
                                        }
                default:
                                        break;
        }

        return TRUE;
}

int main (int   argc,  char *argv[])
{
    /* Initialization of elements needed */
    GstElementFactory *sourceFactory;
    GstElement *pipeline, *source, *parse;
    GstBus *bus;
    guint bus_watch_id;    
    GMainLoop *loop;
    GError *error = NULL;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    /* Initialize the main loop, (replace gst-launch) */
    loop = g_main_loop_new (NULL, FALSE);
    
    /* Create the pipeline */
    pipeline  = gst_pipeline_new ("pipeline");
    if(pipeline  == NULL){
            fprintf (stderr, "error cannot create: %s\n","pipeline" );
            EXIT_FAILURE;        
    }

    /* Create the source, for now we use videosrctest */ 
    sourceFactory = gst_element_factory_find("videotestsrc");
    if( sourceFactory == NULL){
        fprintf (stderr, "error cannot create Factory for: %s\n","videotestsrc" );
        EXIT_FAILURE;        
    }
    source = gst_element_factory_create (sourceFactory, "source");
    if( source == NULL){
       fprintf (stderr, "error cannot create element from factory for: %s\n", (char*) g_type_name (gst_element_factory_get_element_type (sourceFactory)));
       EXIT_FAILURE;        
    }
    
    /* Parse the command line to automatically link rtppay to udpsink */
   parse = gst_parse_bin_from_description("rtpvrawpay ! udpsink host=127.0.0.1 port=1993",TRUE , &error);
   if( error != NULL){
       fprintf (stderr, "error in parsing command line: %s\n",error->message);       
       EXIT_FAILURE;        
    }

    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

    /* add elements to pipeline (and bin if necessary) before linking them */
    gst_bin_add_many(GST_BIN (pipeline),
                     source,
                     parse,
                     NULL);
    
    /* we link the elements together */
    /* videosrctest -> parse ("rtpvrawpay ! udpsink host=127.0.0.1 port=1993") */
    gst_element_link (source, parse);
   
    /* Set the pipeline to "playing" state*/
    g_print ("Now playing\n");
    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);


    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);
    return 0;
}
