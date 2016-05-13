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

/**
 * SECTION:element-videocrop
 * @see_also: #GstVideoBox
 *
 * This element crops video frames, meaning it can remove parts of the
 * picture on the left, right, top or bottom of the picture and output
 * a smaller picture than the input picture, with the unwanted parts at the
 * border removed.
 *
 * The videocrop element is similar to the videobox element, but its main
 * goal is to support a multitude of formats as efficiently as possible.
 * Unlike videbox, it cannot add borders to the picture and unlike videbox
 * it will always output images in exactly the same format as the input image.
 *
 * If there is nothing to crop, the element will operate in pass-through mode.
 *
 * Note that no special efforts are made to handle chroma-subsampled formats
 * in the case of odd-valued cropping and compensate for sub-unit chroma plane
 * shifts for such formats in the case where the #GstVideoCrop:left or
 * #GstVideoCrop:top property is set to an odd number. This doesn't matter for 
 * most use cases, but it might matter for yours.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! videocrop top=42 left=1 right=4 bottom=0 ! ximagesink
 * ]|
 * </refsect2>
 */

/* TODO:
 *  - for packed formats, we could avoid memcpy() in case crop_left
 *    and crop_right are 0 and just create a sub-buffer of the input
 *    buffer
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/video/video.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "../../../../include/mibParameters.h"
#include "../../../../include/videoFormatInfo/videoFormatTable.h"

#include "../../../../include/streaming/plugin/plugin.h"
#include "../../../../include/streaming/plugin/vivoecrop/gstvivoecrop.h"
#include "../../../../include/streaming/plugin/vivoecrop/gstaspectratiocrop.h"



#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_vivoe_crop_debug_category);
#define GST_CAT_DEFAULT gst_vivoe_crop_debug_category

enum
{
  PROP_0,
  PROP_LEFT,
  PROP_RIGHT,
  PROP_TOP,
  PROP_BOTTOM
};

/* we support the same caps as aspectratiocrop (sync changes) */
#define VIVOE_CROP_CAPS                                \
  GST_VIDEO_CAPS_MAKE ("{ RGBx, xRGB, BGRx, xBGR, "    \
      "RGBA, ARGB, BGRA, ABGR, RGB, BGR, AYUV, YUY2, " \
      "YVYU, UYVY, I420, YV12, RGB16, RGB15, GRAY8, "  \
      "NV12, NV21, GRAY16_LE, GRAY16_BE }")

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (VIVOE_CROP_CAPS)
    );

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (VIVOE_CROP_CAPS)
    );

#define gst_vivoe_crop_parent_class parent_class
G_DEFINE_TYPE (GstVivoeCrop, gst_vivoe_crop, GST_TYPE_VIDEO_FILTER);

#if 0
static void gst_vivoe_crop_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_vivoe_crop_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
#endif 

static GstCaps *gst_vivoe_crop_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter_caps);
static gboolean gst_vivoe_crop_src_event (GstBaseTransform * trans,
    GstEvent * event);

static gboolean gst_vivoe_crop_set_info (GstVideoFilter * vfilter, GstCaps * in,
    GstVideoInfo * in_info, GstCaps * out, GstVideoInfo * out_info);
static GstFlowReturn gst_vivoe_crop_transform_frame (GstVideoFilter * vfilter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame);

static gboolean
gst_vivoe_crop_src_event (GstBaseTransform * trans, GstEvent * event)
{
	GstEvent *new_event;
	GstStructure *new_structure;
	const GstStructure *structure;
	const gchar *event_name;
	double pointer_x;
	double pointer_y;

	GstVivoeCrop *vcrop = GST_VIVOE_CROP (trans);
	new_event = NULL;

	GST_OBJECT_LOCK (vcrop);
	if (GST_EVENT_TYPE (event) == GST_EVENT_NAVIGATION &&
			(vcrop->crop_left != 0 || vcrop->crop_top != 0)) {
		structure = gst_event_get_structure (event);
		event_name = gst_structure_get_string (structure, "event");

		if (event_name &&
				(strcmp (event_name, "mouse-move") == 0 ||
				 strcmp (event_name, "mouse-button-press") == 0 ||
				 strcmp (event_name, "mouse-button-release") == 0)) {

			if (gst_structure_get_double (structure, "pointer_x", &pointer_x) &&
					gst_structure_get_double (structure, "pointer_y", &pointer_y)) {

				new_structure = gst_structure_copy (structure);
				gst_structure_set (new_structure,
						"pointer_x", G_TYPE_DOUBLE, (double) (pointer_x + vcrop->crop_left),
						"pointer_y", G_TYPE_DOUBLE, (double) (pointer_y + vcrop->crop_top),
						NULL);

				new_event = gst_event_new_navigation (new_structure);
				gst_event_unref (event);
			} else {
				GST_WARNING_OBJECT (vcrop, "Failed to read navigation event");
			}
		}
	}

	GST_OBJECT_UNLOCK (vcrop);

	return GST_BASE_TRANSFORM_CLASS (parent_class)->src_event (trans,
			(new_event ? new_event : event));
}

static void
gst_vivoe_crop_class_init (GstVivoeCropClass * klass)
{

	GstElementClass *element_class;
	GstBaseTransformClass *basetransform_class;
	GstVideoFilterClass *vfilter_class;


	element_class = (GstElementClass *) klass;
	basetransform_class = (GstBaseTransformClass *) klass;
	vfilter_class = (GstVideoFilterClass *) klass;

	gst_element_class_add_static_pad_template (element_class, &sink_template);
	gst_element_class_add_static_pad_template (element_class, &src_template);
	gst_element_class_set_static_metadata (element_class, "vivoecrop",
			"Filter/Effect/Video",
			"Crops video into a user-defined region",
			"Hoel Vasseur <hoel.vasseur@smile.fr>");

	basetransform_class->transform_caps =
		GST_DEBUG_FUNCPTR (gst_vivoe_crop_transform_caps);
	basetransform_class->src_event = GST_DEBUG_FUNCPTR (gst_vivoe_crop_src_event);

	vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_vivoe_crop_set_info);
	vfilter_class->transform_frame =
		GST_DEBUG_FUNCPTR (gst_vivoe_crop_transform_frame);
}

static void
gst_vivoe_crop_init (GstVivoeCrop * vcrop)
{
	vcrop->crop_right 		= 0;
	vcrop->crop_left 			= 0;
	vcrop->crop_top 			= 0;
	vcrop->crop_bottom 		= 0;
}

#define ROUND_DOWN_2(n)  ((n)&(~1))

static void
gst_vivoe_crop_transform_packed_complex (GstVivoeCrop * vcrop,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
	guint8 *in_data, *out_data;
	guint i, dx;
	gint width, height;
	gint in_stride;
	gint out_stride;

	width = GST_VIDEO_FRAME_WIDTH (out_frame);
	height = GST_VIDEO_FRAME_HEIGHT (out_frame);

	in_data = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
	out_data = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

	in_stride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
	out_stride = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0);

	in_data += vcrop->crop_top * in_stride;

	/* rounding down here so we end up at the start of a macro-pixel and not
	 * in the middle of one */
	in_data += ROUND_DOWN_2 (vcrop->crop_left) *
		GST_VIDEO_FRAME_COMP_PSTRIDE (in_frame, 0);

	dx = width * GST_VIDEO_FRAME_COMP_PSTRIDE (out_frame, 0);

	/* UYVY = 4:2:2 - [U0 Y0 V0 Y1] [U2 Y2 V2 Y3] [U4 Y4 V4 Y5]
	 * YUYV = 4:2:2 - [Y0 U0 Y1 V0] [Y2 U2 Y3 V2] [Y4 U4 Y5 V4] = YUY2 */
	if ((vcrop->crop_left % 2) != 0) {
		for (i = 0; i < height; ++i) {
			gint j;

			memcpy (out_data, in_data, dx);

			/* move just the Y samples one pixel to the left, don't worry about
			 * chroma shift */
			for (j = vcrop->macro_y_off; j < out_stride - 2; j += 2)
				out_data[j] = in_data[j + 2];

			in_data += in_stride;
			out_data += out_stride;
		}
	} else {
		for (i = 0; i < height; ++i) {
			memcpy (out_data, in_data, dx);
			in_data += in_stride;
			out_data += out_stride;
		}
	}
}

static void
gst_vivoe_crop_transform_packed_simple (GstVivoeCrop * vcrop,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
	guint8 *in_data, *out_data;
	gint width, height;
	guint i, dx;
	gint in_stride, out_stride;

	width = GST_VIDEO_FRAME_WIDTH (out_frame);
	height = GST_VIDEO_FRAME_HEIGHT (out_frame);

	in_data = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
	out_data = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

	in_stride = GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
	out_stride = GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0);

	in_data += vcrop->crop_top * in_stride;
	in_data += vcrop->crop_left * GST_VIDEO_FRAME_COMP_PSTRIDE (in_frame, 0);

	dx = width * GST_VIDEO_FRAME_COMP_PSTRIDE (out_frame, 0);

	for (i = 0; i < height; ++i) {
		memcpy (out_data, in_data, dx);
		in_data += in_stride;
		out_data += out_stride;
	}
}

static void
gst_vivoe_crop_transform_planar (GstVivoeCrop * vcrop,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
	gint width, height;
	guint8 *y_out, *u_out, *v_out;
	guint8 *y_in, *u_in, *v_in;
	guint i, dx;

	width = GST_VIDEO_FRAME_WIDTH (out_frame);
	height = GST_VIDEO_FRAME_HEIGHT (out_frame);

	/* Y plane */
	y_in = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
	y_out = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

	y_in +=
		(vcrop->crop_top * GST_VIDEO_FRAME_PLANE_STRIDE (in_frame,
														 0)) + vcrop->crop_left;
	dx = width;

	for (i = 0; i < height; ++i) {
		memcpy (y_out, y_in, dx);
		y_in += GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
		y_out += GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0);
	}

	/* U + V planes */
	u_in = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 1);
	u_out = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 1);

	u_in += (vcrop->crop_top / 2) * GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 1);
	u_in += vcrop->crop_left / 2;

	v_in = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 2);
	v_out = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 2);

	v_in += (vcrop->crop_top / 2) * GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 2);
	v_in += vcrop->crop_left / 2;

	dx = GST_ROUND_UP_2 (width) / 2;

	for (i = 0; i < GST_ROUND_UP_2 (height) / 2; ++i) {
		memcpy (u_out, u_in, dx);
		memcpy (v_out, v_in, dx);
		u_in += GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 1);
		u_out += GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 1);
		v_in += GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 2);
		v_out += GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 2);
	}
}

static void
gst_vivoe_crop_transform_semi_planar (GstVivoeCrop * vcrop,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
	gint width, height;
	guint8 *y_out, *uv_out;
	guint8 *y_in, *uv_in;
	guint i, dx;

	width = GST_VIDEO_FRAME_WIDTH (out_frame);
	height = GST_VIDEO_FRAME_HEIGHT (out_frame);

	/* Y plane */
	y_in = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 0);
	y_out = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 0);

	/* UV plane */
	uv_in = GST_VIDEO_FRAME_PLANE_DATA (in_frame, 1);
	uv_out = GST_VIDEO_FRAME_PLANE_DATA (out_frame, 1);

	y_in += vcrop->crop_top * GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0) +
		vcrop->crop_left;
	dx = width;

	for (i = 0; i < height; ++i) {
		memcpy (y_out, y_in, dx);
		y_in += GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 0);
		y_out += GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 0);
	}

	uv_in += (vcrop->crop_top / 2) * GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 1);
	uv_in += GST_ROUND_DOWN_2 (vcrop->crop_left);
	dx = GST_ROUND_UP_2 (width);

	for (i = 0; i < GST_ROUND_UP_2 (height) / 2; i++) {
		memcpy (uv_out, uv_in, dx);
		uv_in += GST_VIDEO_FRAME_PLANE_STRIDE (in_frame, 1);
		uv_out += GST_VIDEO_FRAME_PLANE_STRIDE (out_frame, 1);
	}
}

static GstFlowReturn
gst_vivoe_crop_transform_frame (GstVideoFilter * vfilter,
    GstVideoFrame * in_frame, GstVideoFrame * out_frame)
{
	GstVivoeCrop *vcrop = GST_VIVOE_CROP (vfilter);

	if (G_UNLIKELY (vcrop->need_update)) {
		if (!gst_vivoe_crop_set_info (vfilter, NULL, &vcrop->in_info, NULL,
					&vcrop->out_info)) {
			return GST_FLOW_ERROR;
		}
	}

	switch (vcrop->packing) {
		case VIVOE_CROP_PIXEL_FORMAT_PACKED_SIMPLE:
			gst_vivoe_crop_transform_packed_simple (vcrop, in_frame, out_frame);
			break;
		case VIVOE_CROP_PIXEL_FORMAT_PACKED_COMPLEX:
			gst_vivoe_crop_transform_packed_complex (vcrop, in_frame, out_frame);
			break;
		case VIVOE_CROP_PIXEL_FORMAT_PLANAR:
			gst_vivoe_crop_transform_planar (vcrop, in_frame, out_frame);
			break;
		case VIVOE_CROP_PIXEL_FORMAT_SEMI_PLANAR:
			gst_vivoe_crop_transform_semi_planar (vcrop, in_frame, out_frame);
			break;
		default:
			g_assert_not_reached ();
	}

	return GST_FLOW_OK;
}

static gint
gst_vivoe_crop_transform_dimension (gint val, gint delta)
{
	gint64 new_val = (gint64) val + (gint64) delta;

	new_val = CLAMP (new_val, 1, G_MAXINT);

	return (gint) new_val;
}

static gboolean
gst_vivoe_crop_transform_dimension_value (const GValue * src_val,
    gint delta, GValue * dest_val, GstPadDirection direction, gboolean dynamic)
{
	gboolean ret = TRUE;

	if (G_VALUE_HOLDS_INT (src_val)) {
		gint ival = g_value_get_int (src_val);
		ival = gst_vivoe_crop_transform_dimension (ival, delta);

		if (dynamic) {
			if (direction == GST_PAD_SRC) {
				if (ival == G_MAXINT) {
					g_value_init (dest_val, G_TYPE_INT);
					g_value_set_int (dest_val, ival);
				} else {
					g_value_init (dest_val, GST_TYPE_INT_RANGE);
					gst_value_set_int_range (dest_val, ival, G_MAXINT);
				}
			} else {
				if (ival == 1) {
					g_value_init (dest_val, G_TYPE_INT);
					g_value_set_int (dest_val, ival);
				} else {
					g_value_init (dest_val, GST_TYPE_INT_RANGE);
					gst_value_set_int_range (dest_val, 1, ival);
				}
			}
		} else {
			g_value_init (dest_val, G_TYPE_INT);
			g_value_set_int (dest_val, ival);
		}
	} else if (GST_VALUE_HOLDS_INT_RANGE (src_val)) {
		gint min = gst_value_get_int_range_min (src_val);
		gint max = gst_value_get_int_range_max (src_val);

		min = gst_vivoe_crop_transform_dimension (min, delta);
		max = gst_vivoe_crop_transform_dimension (max, delta);

		if (dynamic) {
			if (direction == GST_PAD_SRC)
				max = G_MAXINT;
			else
				min = 1;
		}

		if (min == max) {
			g_value_init (dest_val, G_TYPE_INT);
			g_value_set_int (dest_val, min);
		} else {
			g_value_init (dest_val, GST_TYPE_INT_RANGE);
			gst_value_set_int_range (dest_val, min, max);
		}
	} else if (GST_VALUE_HOLDS_LIST (src_val)) {
		gint i;

		g_value_init (dest_val, GST_TYPE_LIST);

		for (i = 0; i < gst_value_list_get_size (src_val); ++i) {
			const GValue *list_val;
			GValue newval = { 0, };

			list_val = gst_value_list_get_value (src_val, i);
			if (gst_vivoe_crop_transform_dimension_value (list_val, delta, &newval,
						direction, dynamic))
				gst_value_list_append_value (dest_val, &newval);
			g_value_unset (&newval);
		}

		if (gst_value_list_get_size (dest_val) == 0) {
			g_value_unset (dest_val);
			ret = FALSE;
		}
	} else {
		ret = FALSE;
	}

	return ret;
}

static GstCaps *
gst_vivoe_crop_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter_caps)
{
	GstVivoeCrop *vcrop;
	GstCaps *other_caps;
	gint dy, dx, i, left, right, bottom, top;
	gboolean w_dynamic, h_dynamic;

	vcrop = GST_VIVOE_CROP (trans);

	GST_OBJECT_LOCK (vcrop);

	GST_LOG_OBJECT (vcrop, "l=%d,r=%d,b=%d,t=%d",
			vcrop->prop_left, vcrop->prop_right, vcrop->prop_bottom, vcrop->prop_top);

	w_dynamic = (vcrop->prop_left == -1 || vcrop->prop_right == -1);
	h_dynamic = (vcrop->prop_top == -1 || vcrop->prop_bottom == -1);

	left = (vcrop->prop_left == -1) ? 0 : vcrop->prop_left;
	right = (vcrop->prop_right == -1) ? 0 : vcrop->prop_right;
	bottom = (vcrop->prop_bottom == -1) ? 0 : vcrop->prop_bottom;
	top = (vcrop->prop_top == -1) ? 0 : vcrop->prop_top;

	GST_OBJECT_UNLOCK (vcrop);

	if (direction == GST_PAD_SRC) {
		dx = left + right;
		dy = top + bottom;
	} else {
		dx = 0 - (left + right);
		dy = 0 - (top + bottom);
	}

	GST_LOG_OBJECT (vcrop, "transforming caps %" GST_PTR_FORMAT, caps);

	other_caps = gst_caps_new_empty ();

	for (i = 0; i < gst_caps_get_size (caps); ++i) {
		const GValue *v;
		GstStructure *structure, *new_structure;
		GValue w_val = { 0, }, h_val = {
			0,};

		structure = gst_caps_get_structure (caps, i);

		v = gst_structure_get_value (structure, "width");
		if (!gst_vivoe_crop_transform_dimension_value (v, dx, &w_val, direction,
					w_dynamic)) {
			GST_WARNING_OBJECT (vcrop, "could not tranform width value with dx=%d"
					", caps structure=%" GST_PTR_FORMAT, dx, structure);
			continue;
		}

		v = gst_structure_get_value (structure, "height");
		if (!gst_vivoe_crop_transform_dimension_value (v, dy, &h_val, direction,
					h_dynamic)) {
			g_value_unset (&w_val);
			GST_WARNING_OBJECT (vcrop, "could not tranform height value with dy=%d"
					", caps structure=%" GST_PTR_FORMAT, dy, structure);
			continue;
		}

		new_structure = gst_structure_copy (structure);
		gst_structure_set_value (new_structure, "width", &w_val);
		gst_structure_set_value (new_structure, "height", &h_val);
		g_value_unset (&w_val);
		g_value_unset (&h_val);
		GST_LOG_OBJECT (vcrop, "transformed structure %2d: %" GST_PTR_FORMAT
				" => %" GST_PTR_FORMAT, i, structure, new_structure);
		gst_caps_append_structure (other_caps, new_structure);
	}

	if (!gst_caps_is_empty (other_caps) && filter_caps) {
		GstCaps *tmp = gst_caps_intersect_full (filter_caps, other_caps,
				GST_CAPS_INTERSECT_FIRST);
		gst_caps_replace (&other_caps, tmp);
		gst_caps_unref (tmp);
	}

	return other_caps;
}

static gboolean
gst_vivoe_crop_set_info (GstVideoFilter * vfilter, GstCaps * in,
    GstVideoInfo * in_info, GstCaps * out, GstVideoInfo * out_info)
{
	GstVivoeCrop *crop = GST_VIVOE_CROP (vfilter);
	int dx, dy;

	GST_OBJECT_LOCK (crop);
	crop->need_update = FALSE;
	crop->crop_left = crop->prop_left;
	crop->crop_right = crop->prop_right;
	crop->crop_top = crop->prop_top;
	crop->crop_bottom = crop->prop_bottom;
	GST_OBJECT_UNLOCK (crop);

	dx = GST_VIDEO_INFO_WIDTH (in_info) - GST_VIDEO_INFO_WIDTH (out_info);
	dy = GST_VIDEO_INFO_HEIGHT (in_info) - GST_VIDEO_INFO_HEIGHT (out_info);

	if (crop->crop_left == -1 && crop->crop_right == -1) {
		crop->crop_left = dx / 2;
		crop->crop_right = dx / 2 + (dx & 1);
	} else if (crop->crop_left == -1) {
		if (G_UNLIKELY (crop->crop_right > dx))
			goto cropping_too_much;
		crop->crop_left = dx - crop->crop_right;
	} else if (crop->crop_right == -1) {
		if (G_UNLIKELY (crop->crop_left > dx))
			goto cropping_too_much;
		crop->crop_right = dx - crop->crop_left;
	}

	if (crop->crop_top == -1 && crop->crop_bottom == -1) {
		crop->crop_top = dy / 2;
		crop->crop_bottom = dy / 2 + (dy & 1);
	} else if (crop->crop_top == -1) {
		if (G_UNLIKELY (crop->crop_bottom > dy))
			goto cropping_too_much;
		crop->crop_top = dy - crop->crop_bottom;
	} else if (crop->crop_bottom == -1) {
		if (G_UNLIKELY (crop->crop_top > dy))
			goto cropping_too_much;
		crop->crop_bottom = dy - crop->crop_top;
	}

	if (G_UNLIKELY ((crop->crop_left + crop->crop_right) >=
				GST_VIDEO_INFO_WIDTH (in_info)
				|| (crop->crop_top + crop->crop_bottom) >=
				GST_VIDEO_INFO_HEIGHT (in_info)))
		goto cropping_too_much;

	if (in && out)
		GST_LOG_OBJECT (crop, "incaps = %" GST_PTR_FORMAT ", outcaps = %"
				GST_PTR_FORMAT, in, out);

	if ((crop->crop_left | crop->crop_right | crop->crop_top | crop->
				crop_bottom) == 0) {
		GST_LOG_OBJECT (crop, "we are using passthrough");
		gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (crop), TRUE);
	} else {
		GST_LOG_OBJECT (crop, "we are not using passthrough");
		gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (crop), FALSE);
	}

	if (GST_VIDEO_INFO_IS_RGB (in_info)
			|| GST_VIDEO_INFO_IS_GRAY (in_info)) {
		crop->packing = VIVOE_CROP_PIXEL_FORMAT_PACKED_SIMPLE;
	} else {
		switch (GST_VIDEO_INFO_FORMAT (in_info)) {
			case GST_VIDEO_FORMAT_AYUV:
				crop->packing = VIVOE_CROP_PIXEL_FORMAT_PACKED_SIMPLE;
				break;
			case GST_VIDEO_FORMAT_YVYU:
			case GST_VIDEO_FORMAT_YUY2:
			case GST_VIDEO_FORMAT_UYVY:
				crop->packing = VIVOE_CROP_PIXEL_FORMAT_PACKED_COMPLEX;
				if (GST_VIDEO_INFO_FORMAT (in_info) == GST_VIDEO_FORMAT_UYVY) {
					/* UYVY = 4:2:2 - [U0 Y0 V0 Y1] [U2 Y2 V2 Y3] [U4 Y4 V4 Y5] */
					crop->macro_y_off = 1;
				} else {
					/* YUYV = 4:2:2 - [Y0 U0 Y1 V0] [Y2 U2 Y3 V2] [Y4 U4 Y5 V4] = YUY2 */
					crop->macro_y_off = 0;
				}
				break;
			case GST_VIDEO_FORMAT_I420:
			case GST_VIDEO_FORMAT_YV12:
				crop->packing = VIVOE_CROP_PIXEL_FORMAT_PLANAR;
				break;
			case GST_VIDEO_FORMAT_NV12:
			case GST_VIDEO_FORMAT_NV21:
				crop->packing = VIVOE_CROP_PIXEL_FORMAT_SEMI_PLANAR;
				break;
			default:
				goto unknown_format;
		}
	}

	crop->in_info = *in_info;
	crop->out_info = *out_info;

	return TRUE;

	/* ERROR */
cropping_too_much:
	{
		GST_WARNING_OBJECT (crop, "we are cropping too much");
		return FALSE;
	}
unknown_format:
	{
		GST_WARNING_OBJECT (crop, "Unsupported format");
		return FALSE;
	}
}

/* called with object lock */
	static inline void
gst_vivoe_crop_set_crop (GstVivoeCrop * vcrop, gint new_value, gint * prop)
{
	if (*prop != new_value) {
		*prop = new_value;
		vcrop->need_update = TRUE;
	}
}


static void 
gst_vivoe_crop_get_roi_values_from_MIB( struct videoFormatTable_entry *videoFormat_entry , gint *top , gint *left , gint *bottom , gint *right, gboolean scalable ){

	/* 
	 * If values are negative (which could happen if a bad value is set to ROI resolution, and top, then set parameters to zero
	 */
	*top 	=  videoFormat_entry->videoFormatRoiOriginTop ;
	if ( top < 0 )
		top = 0 ;

	*left 	=  videoFormat_entry->videoFormatRoiOriginLeft ;
	if ( *left < 0 )
		*left = 0 ;

	if ( scalable )
	{
		*bottom = videoFormat_entry->videoFormatMaxVertRes - ( videoFormat_entry->videoFormatRoiOriginTop 	+ videoFormat_entry->videoFormatRoiExtentBottom ) ;
		*right 	= videoFormat_entry->videoFormatMaxHorzRes - ( videoFormat_entry->videoFormatRoiOriginLeft	+ videoFormat_entry->videoFormatRoiExtentRight  ) ;
	}
	else
	{ 
		*bottom	= videoFormat_entry->videoFormatMaxVertRes - ( videoFormat_entry->videoFormatRoiOriginTop 	+ videoFormat_entry->videoFormatRoiVertRes  ) ;
		*right 	= videoFormat_entry->videoFormatMaxHorzRes - ( videoFormat_entry->videoFormatRoiOriginLeft	+ videoFormat_entry->videoFormatRoiHorzRes  ) ;			
	}

	if ( *bottom < 0 || *bottom >= (videoFormat_entry->videoFormatMaxVertRes - videoFormat_entry->videoFormatRoiOriginTop) )
		*bottom = 0 ;

	if ( *right < 0 || *right >= ( videoFormat_entry->videoFormatMaxHorzRes - videoFormat_entry->videoFormatRoiOriginLeft)  )
		*right = 0 ;

} 

void  gst_vivoe_crop_update (GObject * object, struct videoFormatTable_entry *videoFormat_entry , gboolean scalable )
{
	GstVivoeCrop *vivoe_crop;

	vivoe_crop = GST_VIVOE_CROP (object);

	GST_OBJECT_LOCK (vivoe_crop);

	gint top;
	gint left;
	gint bottom;
	gint right;

	/*
	 * Compute the correct values top, left, bottom and right from values from the MIB
	 */
	gst_vivoe_crop_get_roi_values_from_MIB( videoFormat_entry , &top , &left , &bottom , &right, scalable ); 

	gst_vivoe_crop_set_crop (vivoe_crop, top ,
			&vivoe_crop->prop_top);
	gst_vivoe_crop_set_crop (vivoe_crop, left ,
			&vivoe_crop->prop_left);
	gst_vivoe_crop_set_crop (vivoe_crop, bottom ,
			&vivoe_crop->prop_bottom);
	gst_vivoe_crop_set_crop (vivoe_crop, right ,
			&vivoe_crop->prop_right);

	GST_LOG_OBJECT (vivoe_crop, "l=%d,r=%d,b=%d,t=%d, need_update:%d",
			vivoe_crop->prop_left, vivoe_crop->prop_right, vivoe_crop->prop_bottom,
			vivoe_crop->prop_top, vivoe_crop->need_update);

	GST_OBJECT_UNLOCK (vivoe_crop);

	gst_base_transform_reconfigure_src (GST_BASE_TRANSFORM (vivoe_crop));
}

static gboolean
plugin_init (GstPlugin * plugin)
{
	GST_DEBUG_CATEGORY_INIT (gst_vivoe_crop_debug_category, "vivoecrop", 0, "vivoecrop");

	if (gst_element_register (plugin, "vivoecrop", GST_RANK_NONE,
				GST_TYPE_VIVOE_CROP)
			&& gst_element_register (plugin, "aspectratiocrop", GST_RANK_NONE,
				GST_TYPE_ASPECT_RATIO_CROP))
		return TRUE;

	return FALSE;
}

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
