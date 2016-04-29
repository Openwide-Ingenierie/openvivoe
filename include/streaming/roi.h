/*
 * Licence: GPL
 * Created: Wed, 20 Apr 2016 18:36:29 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef ROI_H
# define ROI_H

roi_data *SP_is_roi(long videoFormatIndex) ;
GstElement *handle_roi( GstElement *pipeline, GstElement *input, struct videoFormatTable_entry *video_stream_info , GstStructure *video_caps );
gboolean update_pipeline_SP_non_scalable_roi_changes( GstElement *pipeline, struct channelTable_entry *channel_entry) ;

#endif /* ROI_H */

