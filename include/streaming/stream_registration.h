/*
 * Licence: GPL
 * Created: Wed, 10 Feb 2016 14:36:51 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef STREAM_REGISTRATION_H
# define STREAM_REGISTRATION_H
struct videoFormatTable_entry; 
void fill_entry(GstStructure* source_str_caps, struct videoFormatTable_entry *video_info);
int initialize_videoFormat(struct videoFormatTable_entry *entry);

#endif /* STREAM_REGISTRATION_H */

