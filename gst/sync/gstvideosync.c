/* GStreamer
 * Copyright (C) 2010 David Schleef <ds@schleef.org>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:element-gstvideosync
 *
 * The videosync element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v fakesrc ! videosync ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <string.h>
#include "gstvideosync.h"

GST_DEBUG_CATEGORY_STATIC (gst_videosync_debug_category);
#define GST_CAT_DEFAULT (gst_videosync_debug_category)

/* prototypes */

static void gst_videosync_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_videosync_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_videosync_dispose (GObject * object);
static void gst_videosync_finalize (GObject * object);

static GstCaps *gst_videosync_get_caps (GstBaseSrc * src);
static gboolean gst_videosync_set_caps (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_videosync_negotiate (GstBaseSrc * src);
static gboolean gst_videosync_newsegment (GstBaseSrc * src);
static gboolean gst_videosync_start (GstBaseSrc * src);
static gboolean gst_videosync_stop (GstBaseSrc * src);
static void
gst_videosync_get_times (GstBaseSrc * src, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);
static gboolean gst_videosync_get_size (GstBaseSrc * src, guint64 * size);
static gboolean gst_videosync_is_seekable (GstBaseSrc * src);
static gboolean gst_videosync_unlock (GstBaseSrc * src);
static gboolean gst_videosync_event (GstBaseSrc * src, GstEvent * event);
static gboolean gst_videosync_do_seek (GstBaseSrc * src, GstSegment * segment);
static gboolean gst_videosync_query (GstBaseSrc * src, GstQuery * query);
static gboolean gst_videosync_check_get_range (GstBaseSrc * src);
static void gst_videosync_fixate (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_videosync_unlock_stop (GstBaseSrc * src);
static gboolean
gst_videosync_prepare_seek_segment (GstBaseSrc * src, GstEvent * seek,
    GstSegment * segment);
static GstFlowReturn gst_videosync_create (GstPushSrc * src, GstBuffer ** buf);

static GstCaps *gst_videosync_sink_getcaps (GstPad * pad);
static gboolean gst_videosync_sink_setcaps (GstPad * pad, GstCaps * caps);
static gboolean gst_videosync_sink_acceptcaps (GstPad * pad, GstCaps * caps);
static void gst_videosync_sink_fixatecaps (GstPad * pad, GstCaps * caps);
static gboolean gst_videosync_sink_activate (GstPad * pad);
static gboolean gst_videosync_sink_activatepush (GstPad * pad, gboolean active);
static gboolean gst_videosync_sink_activatepull (GstPad * pad, gboolean active);
static GstPadLinkReturn gst_videosync_sink_link (GstPad * pad, GstPad * peer);
static void gst_videosync_sink_unlink (GstPad * pad);
static GstFlowReturn gst_videosync_sink_chain (GstPad * pad,
    GstBuffer * buffer);
static GstFlowReturn gst_videosync_sink_chainlist (GstPad * pad,
    GstBufferList * bufferlist);
static gboolean gst_videosync_sink_event (GstPad * pad, GstEvent * event);
static gboolean gst_videosync_sink_query (GstPad * pad, GstQuery * query);
static GstFlowReturn gst_videosync_sink_bufferalloc (GstPad * pad,
    guint64 offset, guint size, GstCaps * caps, GstBuffer ** buf);
static GstIterator *gst_videosync_sink_iterintlink (GstPad * pad);

static GstBuffer *gst_videosync_create_black_image (GstVideosync * videosync);

enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_videosync_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("I420"))
    );

static GstStaticPadTemplate gst_videosync_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("I420"))
    );


/* class initialization */

GST_BOILERPLATE (GstVideosync, gst_videosync, GstPushSrc, GST_TYPE_PUSH_SRC);

static void
gst_videosync_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  GST_DEBUG_CATEGORY_INIT (gst_videosync_debug_category, "videosync", 0,
      "videosync element");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_videosync_sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_videosync_src_template));

  gst_element_class_set_details_simple (element_class, "FIXME Long name",
      "Generic", "FIXME Description", "David Schleef <ds@schleef.org>");
}

static void
gst_videosync_class_init (GstVideosyncClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);
  GstPushSrcClass *push_src_class = GST_PUSH_SRC_CLASS (klass);

  gobject_class->set_property = gst_videosync_set_property;
  gobject_class->get_property = gst_videosync_get_property;
  gobject_class->dispose = gst_videosync_dispose;
  gobject_class->finalize = gst_videosync_finalize;
  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_videosync_get_caps);
  base_src_class->set_caps = GST_DEBUG_FUNCPTR (gst_videosync_set_caps);
  if (0)
    base_src_class->negotiate = GST_DEBUG_FUNCPTR (gst_videosync_negotiate);
  base_src_class->newsegment = GST_DEBUG_FUNCPTR (gst_videosync_newsegment);
  base_src_class->start = GST_DEBUG_FUNCPTR (gst_videosync_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_videosync_stop);
  base_src_class->get_times = GST_DEBUG_FUNCPTR (gst_videosync_get_times);
  base_src_class->get_size = GST_DEBUG_FUNCPTR (gst_videosync_get_size);
  base_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_videosync_is_seekable);
  base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_videosync_unlock);
  base_src_class->event = GST_DEBUG_FUNCPTR (gst_videosync_event);
  if (0)
    base_src_class->do_seek = GST_DEBUG_FUNCPTR (gst_videosync_do_seek);
  base_src_class->query = GST_DEBUG_FUNCPTR (gst_videosync_query);
  base_src_class->check_get_range =
      GST_DEBUG_FUNCPTR (gst_videosync_check_get_range);
  base_src_class->fixate = GST_DEBUG_FUNCPTR (gst_videosync_fixate);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_videosync_unlock_stop);
  base_src_class->prepare_seek_segment =
      GST_DEBUG_FUNCPTR (gst_videosync_prepare_seek_segment);

  push_src_class->create = GST_DEBUG_FUNCPTR (gst_videosync_create);

}

static void
gst_videosync_init (GstVideosync * videosync,
    GstVideosyncClass * videosync_class)
{

  videosync->sinkpad =
      gst_pad_new_from_static_template (&gst_videosync_sink_template, "sink");
  gst_pad_set_getcaps_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_getcaps));
  gst_pad_set_setcaps_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_setcaps));
  gst_pad_set_acceptcaps_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_acceptcaps));
  gst_pad_set_fixatecaps_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_fixatecaps));
  gst_pad_set_activate_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_activate));
  gst_pad_set_activatepush_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_activatepush));
  gst_pad_set_activatepull_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_activatepull));
  gst_pad_set_link_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_link));
  gst_pad_set_unlink_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_unlink));
  gst_pad_set_chain_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_chain));
  gst_pad_set_chain_list_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_chainlist));
  gst_pad_set_event_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_event));
  gst_pad_set_query_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_query));
  if (0)
    gst_pad_set_bufferalloc_function (videosync->sinkpad,
        GST_DEBUG_FUNCPTR (gst_videosync_sink_bufferalloc));
  gst_pad_set_iterate_internal_links_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_iterintlink));
  gst_element_add_pad (GST_ELEMENT (videosync), videosync->sinkpad);

  videosync->srcpad =
      gst_pad_new_from_static_template (&gst_videosync_src_template, "src");

  gst_base_src_set_live (GST_BASE_SRC (videosync), TRUE);
  gst_base_src_set_format (GST_BASE_SRC (videosync), GST_FORMAT_TIME);

  videosync->cond = g_cond_new ();
  videosync->lock = g_mutex_new ();
}

void
gst_videosync_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVideosync *videosync;

  g_return_if_fail (GST_IS_VIDEOSYNC (object));
  videosync = GST_VIDEOSYNC (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_videosync_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstVideosync *videosync;

  g_return_if_fail (GST_IS_VIDEOSYNC (object));
  videosync = GST_VIDEOSYNC (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_videosync_dispose (GObject * object)
{
  GstVideosync *videosync;

  g_return_if_fail (GST_IS_VIDEOSYNC (object));
  videosync = GST_VIDEOSYNC (object);

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

void
gst_videosync_finalize (GObject * object)
{
  GstVideosync *videosync;

  g_return_if_fail (GST_IS_VIDEOSYNC (object));
  videosync = GST_VIDEOSYNC (object);

  /* clean up object here */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GstCaps *
gst_videosync_get_caps (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);
  GstCaps *peercaps;
  GstCaps *caps;

  GST_DEBUG_OBJECT (videosync, "get_caps");

  peercaps = gst_pad_peer_get_caps_reffed (videosync->sinkpad);
  if (peercaps) {
    caps =
        gst_caps_intersect (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD
            (videosync)), peercaps);
    gst_caps_unref (peercaps);
  } else {
    caps =
        gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD
            (videosync)));
  }

  return caps;
}

static gboolean
gst_videosync_set_caps (GstBaseSrc * src, GstCaps * caps)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);
  gboolean ret;
  GstVideoFormat format;
  int width;
  int height;
  int fps_n;
  int fps_d;
  gboolean interlaced;
  int par_n;
  int par_d;

  GST_DEBUG_OBJECT (videosync, "set_caps");

#if 0
  if (gst_pad_is_linked (videosync->sinkpad)) {
    ret = gst_pad_set_caps (videosync->sinkpad, caps);
    if (!ret)
      return ret;
  }
#endif

  ret = gst_video_format_parse_caps (caps, &format, &width, &height);
  ret &= gst_video_parse_caps_framerate (caps, &fps_n, &fps_d);
  par_n = 1;
  par_d = 1;
  gst_video_parse_caps_pixel_aspect_ratio (caps, &par_n, &par_d);
  interlaced = FALSE;
  gst_video_format_parse_caps_interlaced (caps, &interlaced);

  if (ret) {
    videosync->timestamp_offset = videosync->current_time;
    videosync->n_frames = 0;

    videosync->format = format;
    videosync->width = width;
    videosync->height = height;
    videosync->fps_n = fps_n;
    videosync->fps_d = fps_d;
    videosync->par_n = par_n;
    videosync->par_d = par_d;
    videosync->interlaced = interlaced;

    GST_ERROR ("got new caps %dx%d %d/%d", width, height, fps_n, fps_d);
  }

  return ret;
}

static gboolean
gst_videosync_negotiate (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "negotiate");

  return TRUE;
}

static gboolean
gst_videosync_newsegment (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "newsegment");

  return TRUE;
}

static gboolean
gst_videosync_start (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "start");

  videosync->reset = FALSE;

  return TRUE;
}

static gboolean
gst_videosync_stop (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "stop");

  return TRUE;
}

static void
gst_videosync_get_times (GstBaseSrc * src, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "get_times");

  /* for live sources, sync on the timestamp of the buffer */
  if (gst_base_src_is_live (src)) {
    GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

    if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
      /* get duration to calculate end time */
      GstClockTime duration = GST_BUFFER_DURATION (buffer);

      if (GST_CLOCK_TIME_IS_VALID (duration)) {
        *end = timestamp + duration;
      }
      *start = timestamp;
    }
  } else {
    *start = -1;
    *end = -1;
  }
}

static gboolean
gst_videosync_get_size (GstBaseSrc * src, guint64 * size)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "get_size");

  if (videosync->format != GST_VIDEO_FORMAT_UNKNOWN) {
    *size = gst_video_format_get_size (videosync->format,
        videosync->width, videosync->height);
    return TRUE;
  }

  return FALSE;
}

static gboolean
gst_videosync_is_seekable (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "is_seekable");

  return FALSE;
}

static gboolean
gst_videosync_unlock (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "unlock");

  g_mutex_lock (videosync->lock);
  videosync->reset = TRUE;
  g_cond_signal (videosync->cond);
  g_mutex_unlock (videosync->lock);

  return TRUE;
}

static gboolean
gst_videosync_event (GstBaseSrc * src, GstEvent * event)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);
  gboolean res;

  GST_DEBUG_OBJECT (videosync, "event: %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_LATENCY:
      res = TRUE;
      break;
    default:
      res = gst_pad_push_event (videosync->sinkpad, gst_event_ref (event));
      break;
  }

  return res;
}

static gboolean
gst_videosync_do_seek (GstBaseSrc * src, GstSegment * segment)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "do_seek");

  return FALSE;
}

static gboolean
gst_videosync_query (GstBaseSrc * src, GstQuery * query)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);
  gboolean ret;

  GST_DEBUG_OBJECT (videosync, "query: %s", GST_QUERY_TYPE_NAME (query));

  {
    static int x = 20;
    if (x-- == 0) {
      //g_assert_not_reached ();
    }
  }
  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:
      gst_query_set_latency (query, TRUE, 0, 0);
      ret = TRUE;
      break;
    default:
      ret = FALSE;
      break;
  }

  return ret;
}

static gboolean
gst_videosync_check_get_range (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "get_range");

  return FALSE;
}

static void
gst_videosync_fixate (GstBaseSrc * src, GstCaps * caps)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);
  GstStructure *structure;

  GST_DEBUG_OBJECT (videosync, "fixate");

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "width", 320);
  gst_structure_fixate_field_nearest_int (structure, "height", 240);
  gst_structure_fixate_field_nearest_fraction (structure, "framerate", 30, 1);
  if (gst_structure_has_field (structure, "pixel-aspect-ratio"))
    gst_structure_fixate_field_nearest_fraction (structure,
        "pixel-aspect-ratio", 1, 1);
  if (gst_structure_has_field (structure, "color-matrix"))
    gst_structure_fixate_field_string (structure, "color-matrix", "sdtv");
  if (gst_structure_has_field (structure, "chroma-site"))
    gst_structure_fixate_field_string (structure, "chroma-site", "mpeg2");

  if (gst_structure_has_field (structure, "interlaced"))
    gst_structure_fixate_field_boolean (structure, "interlaced", FALSE);

}

static gboolean
gst_videosync_unlock_stop (GstBaseSrc * src)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "unlock_stop");

  return TRUE;
}

static gboolean
gst_videosync_prepare_seek_segment (GstBaseSrc * src, GstEvent * seek,
    GstSegment * segment)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);

  GST_DEBUG_OBJECT (videosync, "seek_segment");

  return FALSE;
}

static GstFlowReturn
gst_videosync_create (GstPushSrc * src, GstBuffer ** buf)
{
  GstVideosync *videosync = GST_VIDEOSYNC (src);
  GstBuffer *buffer;
  GstClockTime current_time;
  GstClockTime next_time;

  current_time = videosync->timestamp_offset +
      gst_util_uint64_scale_int (videosync->n_frames * GST_SECOND,
      videosync->fps_d, videosync->fps_n);
  next_time = videosync->timestamp_offset +
      gst_util_uint64_scale_int ((videosync->n_frames + 1) * GST_SECOND,
      videosync->fps_d, videosync->fps_n);

  GST_DEBUG ("current_time %" GST_TIME_FORMAT, GST_TIME_ARGS (current_time));

  buffer = NULL;
  g_mutex_lock (videosync->lock);
  if (videosync->current_buffer) {
    GstClockTime buffer_start;
    GstClockTime buffer_end;

    buffer_start = GST_BUFFER_TIMESTAMP (videosync->current_buffer) +
        videosync->ts_offset;
    if (GST_BUFFER_DURATION (videosync->current_buffer) != GST_CLOCK_TIME_NONE) {
      buffer_end =
          buffer_start + GST_BUFFER_DURATION (videosync->current_buffer);
    } else {
      buffer_end = buffer_start;
    }

    if (current_time >= buffer_start && current_time <= buffer_end + GST_SECOND) {
      buffer = gst_buffer_ref (videosync->current_buffer);
    }
  }
  videosync->current_time = next_time;
  g_cond_signal (videosync->cond);
  g_mutex_unlock (videosync->lock);

  if (buffer == NULL) {
    buffer = gst_videosync_create_black_image (videosync);
  }

  buffer = gst_buffer_make_metadata_writable (buffer);

  g_assert (videosync->fps_n != 0);

  GST_BUFFER_TIMESTAMP (buffer) = current_time;
  GST_BUFFER_DURATION (buffer) = next_time - current_time;
  GST_BUFFER_OFFSET (buffer) = videosync->n_frames;
  GST_BUFFER_OFFSET_END (buffer) = videosync->n_frames + 1;

  gst_buffer_set_caps (buffer, GST_PAD_CAPS (GST_BASE_SRC_PAD (videosync)));

  videosync->n_frames++;

  *buf = buffer;

  return GST_FLOW_OK;
}

static GstCaps *
gst_videosync_sink_getcaps (GstPad * pad)
{
  GstVideosync *videosync;
  GstCaps *peercaps;
  GstCaps *caps;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "getcaps");

  peercaps = gst_pad_peer_get_caps_reffed (GST_BASE_SRC_PAD (videosync));
  if (peercaps) {
    caps =
        gst_caps_intersect (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD
            (videosync)), peercaps);
    gst_caps_unref (peercaps);
  } else {
    caps =
        gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD
            (videosync)));
  }

  gst_object_unref (videosync);
  return caps;
}

static gboolean
gst_videosync_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstVideosync *videosync;
  gboolean ret;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "setcaps");

  ret = gst_pad_set_caps (GST_BASE_SRC_PAD (videosync), caps);

  gst_object_unref (videosync);
  return ret;
}

static gboolean
gst_videosync_sink_acceptcaps (GstPad * pad, GstCaps * caps)
{
  GstVideosync *videosync;
  gboolean ret;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "acceptcaps");

  ret = gst_pad_accept_caps (GST_BASE_SRC_PAD (videosync), caps);

  gst_object_unref (videosync);
  return ret;
}

static void
gst_videosync_sink_fixatecaps (GstPad * pad, GstCaps * caps)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "fixatecaps");

  gst_object_unref (videosync);
}

static gboolean
gst_videosync_sink_activate (GstPad * pad)
{
  GstVideosync *videosync;
  gboolean ret;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "activate");

  if (gst_pad_check_pull_range (pad)) {
    GST_DEBUG_OBJECT (pad, "activating pull");
    ret = gst_pad_activate_pull (pad, TRUE);
  } else {
    GST_DEBUG_OBJECT (pad, "activating push");
    ret = gst_pad_activate_push (pad, TRUE);
  }

  gst_object_unref (videosync);
  return ret;
}

static gboolean
gst_videosync_sink_activatepush (GstPad * pad, gboolean active)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "activatepush %d", active);


  gst_object_unref (videosync);
  return TRUE;
}

static gboolean
gst_videosync_sink_activatepull (GstPad * pad, gboolean active)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "activatepull %d", active);


  gst_object_unref (videosync);
  return TRUE;
}

static GstPadLinkReturn
gst_videosync_sink_link (GstPad * pad, GstPad * peer)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "link");


  gst_object_unref (videosync);
  return GST_PAD_LINK_OK;
}

static void
gst_videosync_sink_unlink (GstPad * pad)
{
#if 0
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "unlink");


  gst_object_unref (videosync);
#endif
}

static GstFlowReturn
gst_videosync_sink_chain (GstPad * pad, GstBuffer * buffer)
{
  GstVideosync *videosync;
  GstClockTime buffer_start;
  GstClockTime buffer_end;
  GstFlowReturn ret = GST_FLOW_OK;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "chain");

  if (GST_BUFFER_IS_DISCONT (buffer)) {
    videosync->ts_offset = videosync->current_time -
        GST_BUFFER_TIMESTAMP (buffer);
    GST_ERROR ("setting ts offset to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (videosync->ts_offset));
  }

  buffer_start = GST_BUFFER_TIMESTAMP (buffer) + videosync->ts_offset;
  if (GST_BUFFER_DURATION (buffer) != GST_CLOCK_TIME_NONE) {
    buffer_end = buffer_start + GST_BUFFER_DURATION (buffer);
  } else {
    buffer_end = buffer_start;
  }

  g_mutex_lock (videosync->lock);
  while (!videosync->reset && buffer_start > videosync->current_time) {
    GST_INFO ("buffer early, waiting (%" GST_TIME_FORMAT " > %" GST_TIME_FORMAT,
        GST_TIME_ARGS (buffer_start), GST_TIME_ARGS (videosync->current_time));
    /* too early, wait. */
    g_cond_wait (videosync->cond, videosync->lock);
  }
  if (videosync->reset) {
    GST_DEBUG ("got reset");
    ret = GST_FLOW_ERROR;
  }
  if (videosync->current_buffer) {
    gst_buffer_unref (videosync->current_buffer);
  }
  videosync->current_buffer = buffer;
  g_mutex_unlock (videosync->lock);

  gst_object_unref (videosync);
  return ret;
}

static GstFlowReturn
gst_videosync_sink_chainlist (GstPad * pad, GstBufferList * bufferlist)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "chainlist");


  gst_object_unref (videosync);
  return GST_FLOW_OK;
}

static gboolean
gst_videosync_sink_event (GstPad * pad, GstEvent * event)
{
  gboolean res;
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "event type %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      gst_event_unref (event);
      res = TRUE;
      break;
    case GST_EVENT_NEWSEGMENT:
    {
      gboolean update;
      gdouble rate;
      gdouble applied_rate;
      GstFormat format;
      gint64 start;
      gint64 stop;
      gint64 position;

      gst_event_parse_new_segment_full (event, &update, &rate, &applied_rate,
          &format, &start, &stop, &position);
      GST_ERROR ("new segment: update %d, rate %g, applied_rate %g, "
          "format %d, start %" GST_TIME_FORMAT ", stop %" GST_TIME_FORMAT
          ", position %" GST_TIME_FORMAT,
          update, rate, applied_rate, format,
          GST_TIME_ARGS (start), GST_TIME_ARGS (stop),
          GST_TIME_ARGS (position));

#if 0
      gst_segment_set_newsegment_full (&videosync->segment,
          update, rate, applied_rate, format, start, stop, position);
#endif

      gst_event_unref (event);
      res = TRUE;
    }
      break;
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (videosync);
  return res;
}

static gboolean
gst_videosync_sink_query (GstPad * pad, GstQuery * query)
{
  gboolean res;
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "query");

  switch (GST_QUERY_TYPE (query)) {
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

  gst_object_unref (videosync);
  return res;
}

static GstFlowReturn
gst_videosync_sink_bufferalloc (GstPad * pad, guint64 offset, guint size,
    GstCaps * caps, GstBuffer ** buf)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "bufferalloc");


  *buf = gst_buffer_new_and_alloc (size);
  gst_buffer_set_caps (*buf, caps);

  gst_object_unref (videosync);
  return GST_FLOW_OK;
}

static GstIterator *
gst_videosync_sink_iterintlink (GstPad * pad)
{
  GstVideosync *videosync;
  GstIterator *iter;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "iterintlink");

  iter = gst_pad_iterate_internal_links_default (pad);

  gst_object_unref (videosync);
  return iter;
}


static GstBuffer *
gst_videosync_create_black_image (GstVideosync * videosync)
{
  GstBuffer *buffer;
  guint8 *data;
  int size;
  int offset;

  size = gst_video_format_get_size (videosync->format,
      videosync->width, videosync->height);

  buffer = gst_buffer_new_and_alloc (size);
  data = GST_BUFFER_DATA (buffer);

  switch (videosync->format) {
    case GST_VIDEO_FORMAT_I420:
      offset = gst_video_format_get_component_offset (videosync->format,
          1, videosync->width, videosync->height);
      memset (data, 16, offset);
      memset (data + offset, 128, size - offset);
      break;
    default:
      g_assert_not_reached ();
  }

  return buffer;
}
