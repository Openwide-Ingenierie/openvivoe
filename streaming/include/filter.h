/*
 * Licence: GPL
 * Created: Mon, 08 Feb 2016 09:33:04 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef FILTER_H
# define FILTER_H

/* pad templates */


/*
 * First for RAW video we launch gst-inspect-1.0 on rtpvrawpay
 * this are the format the came out
 * so their will be the limit formats for VIVOE (with uncompressed videos)
 * */
#define VIVOE_FORMAT_RAW_FORMATS "{ RGB, RGBA, BGR, BGRA, AYUV, UYVY, I420, Y41B, UYVP }"
#define VIVOE_WIDTH_RAW_RANGE "{576, 704, 720, 768, 1280, 1920}"
#define VIVOE_HEIGHT_RAW_RANGE "{544, 576, 720, 1080}"

#define VIVOE_RAW_CAPS                							\
    "video/x-raw, "                                             \
    "format = (string) " VIVOE_FORMAT_RAW_FORMATS ", "          \
    "width = " VIVOE_WIDTH_RAW_RANGE ", "                       \
    "height = " VIVOE_HEIGHT_RAW_RANGE ", "                     \
    "framerate = " GST_VIDEO_FPS_RANGE

#define VIVOE_MPEG4_CAPS                						\
	"video/mpeg," 												\
	"mpegversion=(int)4, " 										\
    "width = " GST_VIDEO_SIZE_RANGE ", "                        \
    "height = " GST_VIDEO_SIZE_RANGE ", "                       \
    "framerate = " GST_VIDEO_FPS_RANGE

/*#define VIVOE_SIZE_RAW_CAPS { 	"width=576 , height=544", \
								"width=704 , height=576", \
								"width=720 , height=576", \
								"width=768 , height=576", \
								"width=1280 , height=720", \
   								"width=1920 , height=1080"}"*/



gboolean filter_raw( GstElement* input, GstElement* output);
gboolean filter_mp4( GstElement* input, GstElement* output);


#endif /* FILTER_H */

