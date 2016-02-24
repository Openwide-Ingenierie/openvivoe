/*
 * Licence: GPL
 * Created: Tue, 09 Feb 2016 14:58:37 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef STREAM_H
# define STREAM_H

/**
 * \brief structure that register data relative to a stream
 */
typedef struct{
	GstElement 	*pipeline;
	GstBus 		*bus;
    guint 		bus_watch_id;
	int 		videoFormatIndex; /* the index of the videoFormat added in the table*/
}stream_data;

/**
 * \brief an array of stream, that contains all stream initiated, an not deleted yet
 */
GHashTable* streams;

int init_streaming (int   argc,  char *argv[], gpointer main_loop, gpointer stream_datas);
int start_streaming (gpointer stream_datas );
int stop_streaming( gpointer stream_datas );
int delete_steaming_data(gpointer stream_datas );


#endif /* STREAM_H */

