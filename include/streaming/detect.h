/*
 * Licence: GPL
 * Created: Wed, 10 Feb 2016 10:05:36 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef DETECT_H
# define DETECT_H

GstStructure* type_detection(GstBin *pipeline, GstElement *input_video, GMainLoop *loop);

#endif /* DETECT_H */

