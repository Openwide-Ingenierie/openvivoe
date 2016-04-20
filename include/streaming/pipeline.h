/*
 * Licence: GPL
 * Created: Wed, 03 Feb 2016 11:47:34 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef PIPELINE_H
# define PIPELINE_H

void set_udpsink_param( GstElement *udpsink, long channel_entry_index);

GstElement* create_pipeline_videoChannel( 	gpointer stream_datas,
											GstElement* input,
											long videoChannelIndex);


GstElement* create_pipeline_serviceUser( 	gpointer 					stream_datas,
										 	GstCaps 					*caps,
										 	struct channelTable_entry 	*channel_entry, 
										 	gchar 						*cmdline, 
											redirect_data 				*redirect);

GstElement *append_SP_pipeline_for_redirection( GstCaps *caps, 
												long videoFormatIndex);


#endif /* PIPELINE_H */

