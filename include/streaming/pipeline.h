/*
 * Licence: GPL
 * Created: Wed, 03 Feb 2016 11:47:34 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef PIPELINE_H
# define PIPELINE_H

GstElement* create_pipeline_videoChannel( 	gpointer stream_datas,
											GMainLoop *loop,
										 	GstElement* input);
#endif /* PIPELINE_H */

