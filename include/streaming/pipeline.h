/*
 * Licence: GPL
 * Created: Wed, 03 Feb 2016 11:47:34 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef PIPELINE_H
# define PIPELINE_H

GstElement* create_pipeline( GstElement*pipeline, 	GstBus *bus,
							 guint bus_watch_id, 	GMainLoop *loop,
							 GstElement* input, 	char* ip, gint port);
#endif /* PIPELINE_H */

