/*
 * Licence: GPL
 * Created: Tue, 10 May 2016 16:26:09 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef VIVOECAPS_H
# define VIVOECAPS_H




/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *
 * gstidentity.h:
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


#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_VIVOE_CAPS \
  (gst_vivoecaps_get_type())
#define GST_VIVOE_CAPS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIVOE_CAPS,GstVivoeCaps))
#define GST_VIVOE_CAPS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIVOE_CAPS,GstVivoeCapsClass))
#define GST_IS_VIVOE_CAPS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIVOE_CAPS))
#define GST_IS_VIVOE_CAPS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIVOE_CAPS))

typedef struct _GstVivoeCaps GstVivoeCaps;
typedef struct _GstVivoeCapsClass GstVivoeCapsClass;

/**
 * GstVivoeCapsCapsChangeMode:
 * @GST_VIVOE_CAPS_CAPS_CHANGE_MODE_IMMEDIATE: Only accept the current filter caps
 * @GST_VIVOE_CAPS_CAPS_CHANGE_MODE_DELAYED: Temporarily accept previous filter caps
 *
 * Filter caps change behaviour
 */
typedef enum {
  GST_VIVOE_CAPS_CAPS_CHANGE_MODE_IMMEDIATE,
  GST_VIVOE_CAPS_CAPS_CHANGE_MODE_DELAYED
} GstVivoeCapsCapsChangeMode;

/**
 * GstVivoeCaps:
 *
 * The opaque #GstVivoeCaps data structure.
 */
struct _GstVivoeCaps {
  GstBaseTransform trans;

  GstCaps *filter_caps;
  gboolean filter_caps_used;
  GstVivoeCapsCapsChangeMode caps_change_mode;
  gboolean got_sink_caps;

  GList *pending_events;
  GList *previous_caps;
};

struct _GstVivoeCapsClass {
  GstBaseTransformClass trans_class;
};

G_GNUC_INTERNAL GType gst_vivoecaps_get_type (void);

gboolean vivoecaps_init (void) ;

G_END_DECLS

#endif /* VIVOECAPS_H */
