/*
 * Licence: GPL
 * Created: Wed, 10 Feb 2016 10:05:36 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef DETECT_H
# define DETECT_H

GstStructure* type_detection(GstBin *pipeline, GstElement *input_video, GstElement *sink);
GstStructure* type_detection_for_roi(GstBin *pipeline, GstElement *sink );
GstElement *type_detection_element_for_roi( GstBin *pipeline ) ;


#endif /* DETECT_H */

