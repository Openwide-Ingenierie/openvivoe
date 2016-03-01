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
	GstElement 		*pipeline;
	GstBus 			*bus;
    guint 			bus_watch_id;
	rtp_data 		*rtp_datas;
}stream_data;

int init_streaming (gpointer main_loop, gpointer stream_datas, char* format, int width, int height, char* encoding);
int start_streaming (gpointer stream_datas, long channelVideoFormatIndex );
int stop_streaming( gpointer stream_datas, long channelVideoFormatIndex  );
int delete_steaming_data(gpointer stream_datas );


#endif /* STREAM_H */

