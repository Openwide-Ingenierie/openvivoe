/*
 * Licence: GPL
 * Created: Wed, 10 Feb 2016 14:36:51 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef STREAM_REGISTRATION_H
# define STREAM_REGISTRATION_H

#include "../../include/videoFormatInfo/videoFormatTable.h"
typedef struct{
	long 	rtp_type; 			/* for all video Formats */
	int 	clock_rate; 		/* for all video Formats */
	char 	*profile_level_id; 	/* Only for MPEG-4 */
	char 	*config; 			/* Only for MPEG-4 */
}rtp_data ;

/**
 * \brief a definition of all the potential values of the colorspace field of openjpegenc src pad template
 */
#define colo_RGB 		"sRGB"
#define colo_YUV 		"sYUV"
#define colo_GRAY 		"GRAY"

/**
 * \brief the mapped values for videoFormatSampling field of viddeoFormatTable_entry
 */
#define sampling_RGB 	"RGB"
#define sampling_YUV 	"YCbCr-4:2:2"
#define samping_GRAY 	"GRAYSCALE"

gchar* map_colorimetry_to_sampling_j2k(const gchar *colorspace);
gchar* map_sampling_to_colorimetry_j2k(const gchar *sampling);
void fill_entry(GstStructure* source_str_caps, struct videoFormatTable_entry *video_info,gpointer stream_datas);
int initialize_videoFormat(struct videoFormatTable_entry *video_info, gpointer stream_datas,long *channel_entry_index );

#endif /* STREAM_REGISTRATION_H */

