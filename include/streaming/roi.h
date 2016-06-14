/*
 * Licence: GPL
 * Created: Wed, 20 Apr 2016 18:36:29 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef ROI_H
# define ROI_H

gboolean 	handle_roi( GstElement *pipeline,  struct videoFormatTable_entry *video_stream_info, struct channelTable_entry *channel_entry );
gboolean 	update_pipeline_on_roi_changes( gpointer stream_datas , struct channelTable_entry *channel_entry , struct videoFormatTable_entry *videoFormat_entry) ;
gboolean 	update_camera_ctl_on_roi_changes ( struct channelTable_entry *channel_entry ) ;

#endif /* ROI_H */

