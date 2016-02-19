/*
 * Licence: GPL
 * Created: Tue, 09 Feb 2016 14:58:37 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef STREAM_H
# define STREAM_H


typedef struct{
	GstElement 	*pipeline;
	GstBus 		*bus;
    guint 		bus_watch_id;
}stream_data;

int init_streaming (int   argc,  char *argv[], gpointer main_loop, gpointer stream_datas);
int start_streaming(gpointer stream_datas);
int stop_streaming(gpointer stream_datas);
int delete_steaming_data(gpointer stream_datas);

#endif /* STREAM_H */

