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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstvivoecrop
 *
 * The vivoecrop element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! vivoecrop ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/video/video.h>
#include <gstreamer-1.0/gst/video/gstvideofilter.h>
#include "../../include/streaming/gstvivoecrop.h"

GST_DEBUG_CATEGORY_STATIC (gst_vivoe_crop_debug_category);
#define GST_CAT_DEFAULT gst_vivoe_crop_debug_category

/* prototypes */


static void gst_vivoe_crop_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_vivoe_crop_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_vivoe_crop_dispose (GObject * object);
static void gst_vivoe_crop_finalize (GObject * object);

static gboolean gst_vivoe_crop_start (GstBaseTransform * trans);
static gboolean gst_vivoe_crop_stop (GstBaseTransform * trans);
static gboolean gst_vivoe_crop_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_vivoe_crop_transform_frame (GstVideoFilter * filter,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn gst_vivoe_crop_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame);

enum
{
  PROP_0
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstVivoeCrop, gst_vivoe_crop, GST_TYPE_VIDEO_FILTER,
  GST_DEBUG_CATEGORY_INIT (gst_vivoe_crop_debug_category, "vivoecrop", 0,
  "debug category for vivoecrop element"));

static void
gst_vivoe_crop_class_init (GstVivoeCropClass * klass)
{

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
	GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

	/* Setting up pads and setting metadata should be moved to
	   base_class_init if you intend to subclass this class. */
	gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
			gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
				gst_caps_from_string (VIDEO_SRC_CAPS)));
	gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
			gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
				gst_caps_from_string (VIDEO_SINK_CAPS)));

	gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
			"vivoecrop", "Generic", "The Vivoe's gstreamer cropping module",
			"Hoel Vasseur <hoel.vasseur@smile.fr>");

	gobject_class->set_property 			= gst_vivoe_crop_set_property;
	gobject_class->get_property 			= gst_vivoe_crop_get_property;
	gobject_class->dispose 					= gst_vivoe_crop_dispose;
	gobject_class->finalize 				= gst_vivoe_crop_finalize;
	base_transform_class->start 			= GST_DEBUG_FUNCPTR (gst_vivoe_crop_start);
	base_transform_class->stop 				= GST_DEBUG_FUNCPTR (gst_vivoe_crop_stop);
	video_filter_class->set_info 			= GST_DEBUG_FUNCPTR (gst_vivoe_crop_set_info);
//	video_filter_class->transform_frame 	= GST_DEBUG_FUNCPTR (gst_vivoe_crop_transform_frame);
	video_filter_class->transform_frame_ip 	= GST_DEBUG_FUNCPTR (gst_vivoe_crop_transform_frame_ip);

}

static void
gst_vivoe_crop_init (GstVivoeCrop *vivoecrop)
{
}

void
gst_vivoe_crop_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (object);

  GST_DEBUG_OBJECT (vivoecrop, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_vivoe_crop_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (object);

  GST_DEBUG_OBJECT (vivoecrop, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_vivoe_crop_dispose (GObject * object)
{
  GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (object);

  GST_DEBUG_OBJECT (vivoecrop, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_vivoe_crop_parent_class)->dispose (object);
}

void
gst_vivoe_crop_finalize (GObject * object)
{
  GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (object);

  GST_DEBUG_OBJECT (vivoecrop, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_vivoe_crop_parent_class)->finalize (object);
}

static gboolean
gst_vivoe_crop_start (GstBaseTransform * trans)
{
  GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (trans);

  GST_DEBUG_OBJECT (vivoecrop, "start");

  return TRUE;
}

static gboolean
gst_vivoe_crop_stop (GstBaseTransform * trans)
{
  GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (trans);

  GST_DEBUG_OBJECT (vivoecrop, "stop");

  return TRUE;
}

static gboolean
gst_vivoe_crop_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (filter);

  GST_DEBUG_OBJECT (vivoecrop, "set_info");

  return TRUE;
}
#if 0
/* transform */
static GstFlowReturn
gst_vivoe_crop_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{

  GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (filter);

  GST_DEBUG_OBJECT (vivoecrop, "transform_frame");
  
  return GST_FLOW_OK;

}
#endif

static GstFlowReturn
gst_vivoe_crop_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
{

	GstVivoeCrop *vivoecrop = GST_VIVOE_CROP (filter);

	GST_DEBUG_OBJECT (vivoecrop, "transform_frame_ip");

	return GST_FLOW_OK;

}

static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "vivoecrop", GST_RANK_NONE,
      GST_TYPE_VIVOE_CROP);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION 			"1.0"
#endif
#ifndef PACKAGE
#define PACKAGE 			"vivoecrop"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME 		"vivoecrop"
#endif
#ifndef SOURCE
#define SOURCE 				"gstvideocrop.c"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "localhost:openvivoe"
#endif

gboolean
vivoecrop_init (void)
{

	return  gst_plugin_register_static (GST_VERSION_MAJOR,
			GST_VERSION_MINOR,
			"vivoecrop",
			"The Vivoe's gstreamer cropping module",
			plugin_init, 
			VERSION,
			"LGPL", 
			PACKAGE_NAME,
			SOURCE,	
			GST_PACKAGE_ORIGIN
			);

}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		GST_VERSION_MINOR,
		vivoecrop,
		"The Vivoe's gstreamer's crop module",
		plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

