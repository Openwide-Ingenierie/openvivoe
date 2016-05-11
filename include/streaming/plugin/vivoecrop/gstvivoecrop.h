/* GStreamer video frame cropping
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_VIVOE_CROP_H__
#define __GST_VIVOE_CROP_H__

#include <gst/video/gstvideofilter.h>
#include "../../../../include/mibParameters.h"
#include "../../../../include/videoFormatInfo/videoFormatTable.h"

G_BEGIN_DECLS

#define GST_TYPE_VIVOE_CROP \
	(gst_vivoe_crop_get_type())
#define GST_VIVOE_CROP(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIVOE_CROP,GstVivoeCrop))
#define GST_VIVOE_CROP_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIVOE_CROP,GstVivoeCropClass))
#define GST_IS_VIVOE_CROP(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIVOE_CROP))
#define GST_IS_VIVOE_CROP_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIVOE_CROP))

typedef enum {
	VIVOE_CROP_PIXEL_FORMAT_PACKED_SIMPLE = 0,  /* RGBx, AYUV */
	VIVOE_CROP_PIXEL_FORMAT_PACKED_COMPLEX,     /* UYVY, YVYU */
	VIVOE_CROP_PIXEL_FORMAT_PLANAR,             /* I420, YV12 */
	VIVOE_CROP_PIXEL_FORMAT_SEMI_PLANAR         /* NV12, NV21 */
} VivoeCropPixelFormat;

typedef struct _GstVivoeCropImageDetails GstVivoeCropImageDetails;

typedef struct _GstVivoeCrop GstVivoeCrop;
typedef struct _GstVivoeCropClass GstVivoeCropClass;

struct _GstVivoeCrop
{
	GstVideoFilter parent;

	/*< private >*/
	gint prop_left;
	gint prop_right;
	gint prop_top;
	gint prop_bottom;
	gboolean need_update;

	GstVideoInfo in_info;
	GstVideoInfo out_info;

	gint crop_left;
	gint crop_right;
	gint crop_top;
	gint crop_bottom;

	VivoeCropPixelFormat  packing;
	gint macro_y_off;
};

struct _GstVivoeCropClass
{
  GstVideoFilterClass parent_class;
};

GType gst_video_crop_get_type (void);

void
gst_vivoe_crop_set_videoformatindex (GObject * object,  const gint value);
void gst_vivoe_crop_get_videoformatindex (GObject * object, int *value);
void gst_vivoe_crop_update (GObject * object, struct videoFormatTable_entry *videoFormat_entry , gboolean scalable ) ;

gboolean vivoecrop_init (void) ;
G_END_DECLS

#endif /* __GST_VIDEO_CROP_H__ */

