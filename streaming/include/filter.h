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
#define VIDEO_VIVOE_CAPS \
		GST_VIDEO_CAPS_MAKE("{ RGB, RGBA, BGR, BGRA, AYUV, UYVY, I420, Y41B, UYVP }")

gboolean filter_format( GstElement* input, GstElement* output);


#endif /* FILTER_H */

