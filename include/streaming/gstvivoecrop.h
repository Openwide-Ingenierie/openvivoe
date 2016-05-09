/* GStreamer
 * Copyright (C) 2016 FIXME <fixme@example.com>
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

#ifndef _GST_VIVOE_CROP_H_
#define _GST_VIVOE_CROP_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_VIVOE_CROP   (gst_vivoe_crop_get_type())
#define GST_VIVOE_CROP(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIVOE_CROP,GstVivoeCrop))
#define GST_VIVOE_CROP_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIVOE_CROP,GstVivoeCropClass))
#define GST_IS_VIVOE_CROP(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIVOE_CROP))
#define GST_IS_VIVOE_CROP_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIVOE_CROP))

typedef struct _GstVivoeCrop GstVivoeCrop;
typedef struct _GstVivoeCropClass GstVivoeCropClass;

struct _GstVivoeCrop
{
  GstVideoFilter base_vivoecrop;

};

struct _GstVivoeCropClass
{
  GstVideoFilterClass base_vivoecrop_class;
};

GType gst_vivoe_crop_get_type (void);

G_END_DECLS

gboolean
vivoecrop_init (void) ;
/* pad templates */

#define VIDEO_SRC_CAPS \
	GST_VIDEO_CAPS_MAKE("{ I420 }")
  //  GST_VIDEO_CAPS_MAKE("{ RGBx, xRGB, BGRx, xBGR, RGBA, ARGB, BGRA, ABGR, RGB, BGR, AYUV, YUY2, YVYU, UYVY, I420, YV12, RGB16, RGB15, GRAY8, NV12, NV21, GRAY16_LE, GRAY16_BE }")

#define VIDEO_SINK_CAPS \
	GST_VIDEO_CAPS_MAKE("{I420 }")
   // GST_VIDEO_CAPS_MAKE("{ RGBx, xRGB, BGRx, xBGR, RGBA, ARGB, BGRA, ABGR, RGB, BGR, AYUV, YUY2, YVYU, UYVY, I420, YV12, RGB16, RGB15, GRAY8, NV12, NV21, GRAY16_LE, GRAY16_BE }")	

#endif /* VIVOE_CROP_H */
