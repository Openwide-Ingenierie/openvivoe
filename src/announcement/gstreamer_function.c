/*
 * Licence: GPL
 * Created: Wed, 09 Mar 2016 10:24:44 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/sdp/gstsdpmessage.h>
#include <gstreamer-1.0/gst/rtp/gstrtppayloads.h>
#include "../../include/announcement/gstreamer_function.h"

static const gchar *
gst_sdp_get_attribute_for_pt (const GstSDPMedia * media, const gchar * name,
    gint pt)
{
  guint i;

  for (i = 0;; i++) {
    const gchar *attr;
    gint val;

    if ((attr = gst_sdp_media_get_attribute_val_n (media, name, i)) == NULL)
      break;

    if (sscanf (attr, "%d ", &val) != 1)
      continue;

    if (val == pt)
      return attr;
  }
  return NULL;
}

#define PARSE_INT(p, del, res)          \
G_STMT_START {                          \
  gchar *t = p;                         \
  p = strstr (p, del);                  \
  if (p == NULL)                        \
    res = -1;                           \
  else {                                \
    *p = '\0';                          \
    p++;                                \
    res = atoi (t);                     \
  }                                     \
} G_STMT_END

#define PARSE_STRING(p, del, res)       \
G_STMT_START {                          \
  gchar *t = p;                         \
  p = strstr (p, del);                  \
  if (p == NULL) {                      \
    res = NULL;                         \
    p = t;                              \
  }                                     \
  else {                                \
    *p = '\0';                          \
    p++;                                \
    res = t;                            \
  }                                     \
} G_STMT_END

#define SKIP_SPACES(p)                  \
  while (*p && g_ascii_isspace (*p))    \
    p++;

/* rtpmap contains:
 *
 *  <payload> <encoding_name>/<clock_rate>[/<encoding_params>]
 */
static gboolean
gst_sdp_parse_rtpmap (const gchar * rtpmap, gint * payload, gchar ** name,
    gint * rate, gchar ** params)
{
  gchar *p, *t;

  p = (gchar *) rtpmap;

  PARSE_INT (p, " ", *payload);
  if (*payload == -1)
    return FALSE;

  SKIP_SPACES (p);
  if (*p == '\0')
    return FALSE;

  PARSE_STRING (p, "/", *name);
  if (*name == NULL) {
    GST_DEBUG ("no rate, name %s", p);
    /* no rate, assume -1 then, this is not supposed to happen but RealMedia
     * streams seem to omit the rate. */
    *name = p;
    *rate = -1;
    return TRUE;
  }

  t = p;
  p = strstr (p, "/");
  if (p == NULL) {
    *rate = atoi (t);
    return TRUE;
  }
  *p = '\0';
  p++;
  *rate = atoi (t);

  t = p;
  if (*p == '\0')
    return TRUE;
  *params = t;

  return TRUE;
}

/**
 * gst_sdp_media_get_caps_from_media:
 * @media: a #GstSDPMedia
 * @pt: a payload type
 *
 * Mapping of caps from SDP fields:
 *
 * a=rtpmap:(payload) (encoding_name)/(clock_rate)[/(encoding_params)]
 *
 * a=framesize:(payload) (width)-(height)
 *
 * a=fmtp:(payload) (param)[=(value)];...
 *
 * Returns: a #GstCaps, or %NULL if an error happened
 *
 * I got it from Gstreamer source cod, release 1.8
 */
GstCaps *
gst_sdp_media_get_caps_from_media (const GstSDPMedia * media, gint pt)
{
  GstCaps *caps;
  const gchar *rtpmap;
  const gchar *fmtp;
  const gchar *framesize;
  gchar *name = NULL;
  gint rate = -1;
  gchar *params = NULL;
  gchar *tmp;
  GstStructure *s;
  gint payload = 0;
  gboolean ret;

  /* get and parse rtpmap */
  rtpmap = gst_sdp_get_attribute_for_pt (media, "rtpmap", pt);

  if (rtpmap) {
    ret = gst_sdp_parse_rtpmap (rtpmap, &payload, &name, &rate, &params);
    if (!ret) {
      GST_ERROR ("error parsing rtpmap, ignoring");
      rtpmap = NULL;
    }
  }
  /* dynamic payloads need rtpmap or we fail */
  if (rtpmap == NULL && pt >= 96)
    goto no_rtpmap;

  /* check if we have a rate, if not, we need to look up the rate from the
   * default rates based on the payload types. */
  if (rate == -1) {
    const GstRTPPayloadInfo *info;

    if (GST_RTP_PAYLOAD_IS_DYNAMIC (pt)) {
      /* dynamic types, use media and encoding_name */
      tmp = g_ascii_strdown (media->media, -1);
      info = gst_rtp_payload_info_for_name (tmp, name);
      g_free (tmp);
    } else {
      /* static types, use payload type */
      info = gst_rtp_payload_info_for_pt (pt);
    }

    if (info) {
      if ((rate = info->clock_rate) == 0)
        rate = -1;
    }
    /* we fail if we cannot find one */
    if (rate == -1)
      goto no_rate;
  }

  tmp = g_ascii_strdown (media->media, -1);
  caps = gst_caps_new_simple ("application/x-rtp",
      "media", G_TYPE_STRING, tmp, "payload", G_TYPE_INT, pt, NULL);
  g_free (tmp);
  s = gst_caps_get_structure (caps, 0);

  gst_structure_set (s, "clock-rate", G_TYPE_INT, rate, NULL);

  /* encoding name must be upper case */
  if (name != NULL) {
    tmp = g_ascii_strup (name, -1);
    gst_structure_set (s, "encoding-name", G_TYPE_STRING, tmp, NULL);
    g_free (tmp);
  }

  /* params must be lower case */
  if (params != NULL) {
    tmp = g_ascii_strdown (params, -1);
    gst_structure_set (s, "encoding-params", G_TYPE_STRING, tmp, NULL);
    g_free (tmp);
  }

  /* parse optional fmtp: field */
  if ((fmtp = gst_sdp_get_attribute_for_pt (media, "fmtp", pt))) {
    gchar *p;
    gint payload = 0;

    p = (gchar *) fmtp;

    /* p is now of the format <payload> <param>[=<value>];... */
    PARSE_INT (p, " ", payload);
    if (payload != -1 && payload == pt) {
      gchar **pairs;
      gint i;

      /* <param>[=<value>] are separated with ';' */
      pairs = g_strsplit (p, ";", 0);
      for (i = 0; pairs[i]; i++) {
        gchar *valpos;
        const gchar *val, *key;
        gint j;
        const gchar *reserved_keys[] =
            { "media", "payload", "clock-rate", "encoding-name",
          "encoding-params"
        };

        /* the key may not have a '=', the value can have other '='s */
        valpos = strstr (pairs[i], "=");
        if (valpos) {
          /* we have a '=' and thus a value, remove the '=' with \0 */
          *valpos = '\0';
          /* value is everything between '=' and ';'. We split the pairs at ;
           * boundaries so we can take the remainder of the value. Some servers
           * put spaces around the value which we strip off here. Alternatively
           * we could strip those spaces in the depayloaders should these spaces
           * actually carry any meaning in the future. */
          val = g_strstrip (valpos + 1);
        } else {
          /* simple <param>;.. is translated into <param>=1;... */
          val = "1";
        }
        /* strip the key of spaces, convert key to lowercase but not the value. */
        key = g_strstrip (pairs[i]);

        /* skip keys from the fmtp, which we already use ourselves for the
         * caps. Some software is adding random things like clock-rate into
         * the fmtp, and we would otherwise here set a string-typed clock-rate
         * in the caps... and thus fail to create valid RTP caps
         */
        for (j = 0; j < G_N_ELEMENTS (reserved_keys); j++) {
          if (g_ascii_strcasecmp (reserved_keys[j], key) == 0) {
            key = "";
            break;
          }
        }

        if (strlen (key) > 1) {
          tmp = g_ascii_strdown (key, -1);
          gst_structure_set (s, tmp, G_TYPE_STRING, val, NULL);
          g_free (tmp);
        }
      }
      g_strfreev (pairs);
    }
  }

  /* parse framesize: field */
  if ((framesize = gst_sdp_media_get_attribute_val (media, "framesize"))) {
    gchar *p;

    /* p is now of the format <payload> <width>-<height> */
    p = (gchar *) framesize;

    PARSE_INT (p, " ", payload);
    if (payload != -1 && payload == pt) {
      gst_structure_set (s, "a-framesize", G_TYPE_STRING, p, NULL);
    }
  }

  return caps;

  /* ERRORS */
no_rtpmap:
  {
    GST_ERROR ("rtpmap type not given for dynamic payload %d", pt);
    return NULL;
  }
no_rate:
  {
    GST_ERROR ("rate unknown for payload type %d", pt);
    return NULL;
  }
}

