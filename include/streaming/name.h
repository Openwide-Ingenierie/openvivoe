/*
 * Licence: GPL
 * Created: Wed, 04 May 2016 10:08:31 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef NAME_H
# define NAME_H

/*
 * This file is us to register all the names given to the gstreamer's elements
 * because they are many, and used to retrieve an element in a pipeline
 */


/*
 * Elements created in stream.c
 */
#define PIPELINE_NAME 			"pipeline"

#define APPSRC_NAME 			"src-redirection"

#define SOURCE_NAME 			"source"

/*
 * Elements created in pipeline.c
 */
#define RTPRAWPAY_NAME 			"rtpvrawpay"

#define MPEG4PARSER_NAME 		"parser"

#define RTPMP4PAY_NAME 			"rtpmp4vpay"

#define CAPSFITER_J2K_NAME 		"capsfilter-image/x-jpc"

#define RTPJ2KPAY_NAME 			"rtpj2kpay"

#define UDPSINK_NAME 			"udpsink"

#define UDPSRC_NAME 			"udpsrc"

#define RTPRAWDEPAY_NAME 		"rtpvrawdepay"

#define RTPMP4DEPAY_NAME 		"rtpmp4vdepay"

#define RTPJ2KDEPAY_NAME 		"rtpj2kdepay"

#define APPSINK_NAME 			"redirect-sink"

/*
 * Elements created in detect.c
 */
#define SINK_TYPEFIND_NAME 		"sink-typefind"

/*
 * Elements created in roi.c
 */
#define SCLAING_ELT_NAME 		"scaling_element"

#define VIDEOCROP_ROI_NAME 		"videocrop_roi"

#define CAPSFILTER_ROI_NAME 		"capsfilter_roi"


#endif /* NAME_H */

