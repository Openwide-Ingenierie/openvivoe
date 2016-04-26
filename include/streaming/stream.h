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
	GstElement 		*udp_elem;
	GstBus 			*bus;
    guint 			bus_watch_id;
	rtp_data 		*rtp_datas;
}stream_data;

int handle_SP_default_StartUp_mode( long videoFormatIndex );
int init_stream_SP( int videoFormatIndex );
int init_stream_SU( GstCaps *caps, struct channelTable_entry *channel_entry);
gboolean start_streaming (gpointer stream_datas, long channelVideoFormatIndex );
int stop_streaming( gpointer stream_datas, long channelVideoFormatIndex  );
int delete_steaming_data(gpointer channel_entry ); /* need to have chanelEntry because we need to free the sap_data */


#endif /* STREAM_H */

