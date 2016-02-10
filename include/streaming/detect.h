/*
 * Licence: GPL
 * Created: Wed, 10 Feb 2016 10:05:36 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef DETECT_H
# define DETECT_H

#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>

const char* type_detection(GstBin *pipeline, GstElement *input_video, GMainLoop *loop);

#endif /* DETECT_H */

