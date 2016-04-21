/*
 * Licence: GPL
 * Created: Wed, 20 Apr 2016 18:36:29 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef ROI_H
# define ROI_H

GstElement *handle_roi( GstElement *pipeline, GstElement *input, struct videoFormatTable_entry *video_stream_info , GstStructure *video_caps );

#endif /* ROI_H */

