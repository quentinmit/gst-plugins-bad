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
 * SECTION:element-gstaudiosync
 *
 * The audiosync element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v fakesrc ! audiosync ! FIXME ! fakesink
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
#include "gstaudiosync.h"

GST_DEBUG_CATEGORY_STATIC (gst_audiosync_debug_category);
#define GST_CAT_DEFAULT (gst_audiosync_debug_category)

/* prototypes */

static void gst_audiosync_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_audiosync_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_audiosync_dispose (GObject * object);
static void gst_audiosync_finalize (GObject * object);

static GstCaps *gst_audiosync_get_caps (GstBaseSrc * src);
static gboolean gst_audiosync_set_caps (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_audiosync_negotiate (GstBaseSrc * src);
static gboolean gst_audiosync_newsegment (GstBaseSrc * src);
static gboolean gst_audiosync_start (GstBaseSrc * src);
static gboolean gst_audiosync_stop (GstBaseSrc * src);
static void
gst_audiosync_get_times (GstBaseSrc * src, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);
static gboolean gst_audiosync_get_size (GstBaseSrc * src, guint64 * size);
static gboolean gst_audiosync_is_seekable (GstBaseSrc * src);
static gboolean gst_audiosync_unlock (GstBaseSrc * src);
static gboolean gst_audiosync_event (GstBaseSrc * src, GstEvent * event);
static gboolean gst_audiosync_do_seek (GstBaseSrc * src, GstSegment * segment);
static gboolean gst_audiosync_query (GstBaseSrc * src, GstQuery * query);
static gboolean gst_audiosync_check_get_range (GstBaseSrc * src);
static void gst_audiosync_fixate (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_audiosync_unlock_stop (GstBaseSrc * src);
static gboolean
gst_audiosync_prepare_seek_segment (GstBaseSrc * src, GstEvent * seek,
    GstSegment * segment);
static GstFlowReturn gst_audiosync_create (GstPushSrc * src, GstBuffer ** buf);

static GstCaps *gst_audiosync_sink_getcaps (GstPad * pad);
static gboolean gst_audiosync_sink_setcaps (GstPad * pad, GstCaps * caps);
static gboolean gst_audiosync_sink_acceptcaps (GstPad * pad, GstCaps * caps);
static void gst_audiosync_sink_fixatecaps (GstPad * pad, GstCaps * caps);
static gboolean gst_audiosync_sink_activate (GstPad * pad);
static gboolean gst_audiosync_sink_activatepush (GstPad * pad, gboolean active);
static gboolean gst_audiosync_sink_activatepull (GstPad * pad, gboolean active);
static GstPadLinkReturn gst_audiosync_sink_link (GstPad * pad, GstPad * peer);
static void gst_audiosync_sink_unlink (GstPad * pad);
static GstFlowReturn gst_audiosync_sink_chain (GstPad * pad,
    GstBuffer * buffer);
static GstFlowReturn gst_audiosync_sink_chainlist (GstPad * pad,
    GstBufferList * bufferlist);
static gboolean gst_audiosync_sink_event (GstPad * pad, GstEvent * event);
static gboolean gst_audiosync_sink_query (GstPad * pad, GstQuery * query);
static GstFlowReturn gst_audiosync_sink_bufferalloc (GstPad * pad,
    guint64 offset, guint size, GstCaps * caps, GstBuffer ** buf);
static GstIterator *gst_audiosync_sink_iterintlink (GstPad * pad);

static GstBuffer *gst_audiosync_create_silence (GstAudiosync * audiosync);

enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_audiosync_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_AUDIO_INT_STANDARD_PAD_TEMPLATE_CAPS)
    );

static GstStaticPadTemplate gst_audiosync_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_AUDIO_INT_STANDARD_PAD_TEMPLATE_CAPS)
    );


/* class initialization */

GST_BOILERPLATE (GstAudiosync, gst_audiosync, GstPushSrc, GST_TYPE_PUSH_SRC);

static void
gst_audiosync_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  GST_DEBUG_CATEGORY_INIT (gst_audiosync_debug_category, "audiosync", 0,
      "audiosync element");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_audiosync_sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_audiosync_src_template));

  gst_element_class_set_details_simple (element_class, "FIXME Long name",
      "Generic", "FIXME Description", "David Schleef <ds@schleef.org>");
}

static void
gst_audiosync_class_init (GstAudiosyncClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);
  GstPushSrcClass *push_src_class = GST_PUSH_SRC_CLASS (klass);

  gobject_class->set_property = gst_audiosync_set_property;
  gobject_class->get_property = gst_audiosync_get_property;
  gobject_class->dispose = gst_audiosync_dispose;
  gobject_class->finalize = gst_audiosync_finalize;
  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_audiosync_get_caps);
  base_src_class->set_caps = GST_DEBUG_FUNCPTR (gst_audiosync_set_caps);
  if (0)
    base_src_class->negotiate = GST_DEBUG_FUNCPTR (gst_audiosync_negotiate);
  base_src_class->newsegment = GST_DEBUG_FUNCPTR (gst_audiosync_newsegment);
  base_src_class->start = GST_DEBUG_FUNCPTR (gst_audiosync_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_audiosync_stop);
  base_src_class->get_times = GST_DEBUG_FUNCPTR (gst_audiosync_get_times);
  base_src_class->get_size = GST_DEBUG_FUNCPTR (gst_audiosync_get_size);
  base_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_audiosync_is_seekable);
  base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_audiosync_unlock);
  base_src_class->event = GST_DEBUG_FUNCPTR (gst_audiosync_event);
  if (0)
    base_src_class->do_seek = GST_DEBUG_FUNCPTR (gst_audiosync_do_seek);
  base_src_class->query = GST_DEBUG_FUNCPTR (gst_audiosync_query);
  base_src_class->check_get_range =
      GST_DEBUG_FUNCPTR (gst_audiosync_check_get_range);
  base_src_class->fixate = GST_DEBUG_FUNCPTR (gst_audiosync_fixate);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_audiosync_unlock_stop);
  base_src_class->prepare_seek_segment =
      GST_DEBUG_FUNCPTR (gst_audiosync_prepare_seek_segment);

  push_src_class->create = GST_DEBUG_FUNCPTR (gst_audiosync_create);

}

static void
gst_audiosync_init (GstAudiosync * audiosync,
    GstAudiosyncClass * audiosync_class)
{

  audiosync->sinkpad =
      gst_pad_new_from_static_template (&gst_audiosync_sink_template, "sink");
  gst_pad_set_getcaps_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_getcaps));
  gst_pad_set_setcaps_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_setcaps));
  gst_pad_set_acceptcaps_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_acceptcaps));
  gst_pad_set_fixatecaps_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_fixatecaps));
  gst_pad_set_activate_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_activate));
  gst_pad_set_activatepush_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_activatepush));
  gst_pad_set_activatepull_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_activatepull));
  gst_pad_set_link_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_link));
  gst_pad_set_unlink_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_unlink));
  gst_pad_set_chain_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_chain));
  gst_pad_set_chain_list_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_chainlist));
  gst_pad_set_event_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_event));
  gst_pad_set_query_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_query));
  if (0)
    gst_pad_set_bufferalloc_function (audiosync->sinkpad,
        GST_DEBUG_FUNCPTR (gst_audiosync_sink_bufferalloc));
  gst_pad_set_iterate_internal_links_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_iterintlink));
  gst_element_add_pad (GST_ELEMENT (audiosync), audiosync->sinkpad);

  audiosync->srcpad =
      gst_pad_new_from_static_template (&gst_audiosync_src_template, "src");

  gst_base_src_set_live (GST_BASE_SRC (audiosync), TRUE);
  gst_base_src_set_format (GST_BASE_SRC (audiosync), GST_FORMAT_TIME);

  audiosync->cond = g_cond_new ();
  audiosync->lock = g_mutex_new ();
}

void
gst_audiosync_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudiosync *audiosync;

  g_return_if_fail (GST_IS_AUDIOSYNC (object));
  audiosync = GST_AUDIOSYNC (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_audiosync_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudiosync *audiosync;

  g_return_if_fail (GST_IS_AUDIOSYNC (object));
  audiosync = GST_AUDIOSYNC (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_audiosync_dispose (GObject * object)
{
  GstAudiosync *audiosync;

  g_return_if_fail (GST_IS_AUDIOSYNC (object));
  audiosync = GST_AUDIOSYNC (object);

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

void
gst_audiosync_finalize (GObject * object)
{
  GstAudiosync *audiosync;

  g_return_if_fail (GST_IS_AUDIOSYNC (object));
  audiosync = GST_AUDIOSYNC (object);

  /* clean up object here */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GstCaps *
gst_audiosync_get_caps (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);
  GstCaps *peercaps;
  GstCaps *caps;

  GST_ERROR_OBJECT (audiosync, "get_caps");

  peercaps = gst_pad_peer_get_caps_reffed (audiosync->sinkpad);
  if (peercaps) {
    caps =
        gst_caps_intersect (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD
            (audiosync)), peercaps);
    gst_caps_unref (peercaps);
  } else {
    caps =
        gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD
            (audiosync)));
  }

  return caps;
}

static gboolean
gst_audiosync_set_caps (GstBaseSrc * src, GstCaps * caps)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);
  gboolean ret = TRUE;
  GstStructure *structure;
  int rate;
  int channels;

  GST_ERROR_OBJECT (audiosync, "set_caps");

  structure = gst_caps_get_structure (caps, 0);
  ret = gst_structure_get_int (structure, "channels", &channels);
  ret &= gst_structure_get_int (structure, "rate", &rate);

  if (ret) {
    audiosync->timestamp_offset = audiosync->current_time;
    audiosync->n_samples = 0;

    audiosync->channels = channels;
    audiosync->rate = rate;

    GST_ERROR ("got new caps rate=%d channels=%d", rate, channels);
  }

  return ret;
}

static gboolean
gst_audiosync_negotiate (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "negotiate");

  return TRUE;
}

static gboolean
gst_audiosync_newsegment (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "newsegment");

  return TRUE;
}

static gboolean
gst_audiosync_start (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "start");

  audiosync->reset = FALSE;

  return TRUE;
}

static gboolean
gst_audiosync_stop (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "stop");

  return TRUE;
}

static void
gst_audiosync_get_times (GstBaseSrc * src, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "get_times");

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
gst_audiosync_get_size (GstBaseSrc * src, guint64 * size)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "get_size");

  *size = 2 * 2 * 1024;
  return TRUE;
}

static gboolean
gst_audiosync_is_seekable (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "is_seekable");

  return FALSE;
}

static gboolean
gst_audiosync_unlock (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "unlock");

  g_mutex_lock (audiosync->lock);
  audiosync->reset = TRUE;
  g_cond_signal (audiosync->cond);
  g_mutex_unlock (audiosync->lock);

  return TRUE;
}

static gboolean
gst_audiosync_event (GstBaseSrc * src, GstEvent * event)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);
  gboolean res;

  GST_DEBUG_OBJECT (audiosync, "event: %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_LATENCY:
      res = TRUE;
      break;
    default:
      res = gst_pad_push_event (audiosync->sinkpad, gst_event_ref (event));
      break;
  }

  return res;
}

static gboolean
gst_audiosync_do_seek (GstBaseSrc * src, GstSegment * segment)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "do_seek");

  return FALSE;
}

static gboolean
gst_audiosync_query (GstBaseSrc * src, GstQuery * query)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);
  gboolean ret;

  GST_DEBUG_OBJECT (audiosync, "query: %s", GST_QUERY_TYPE_NAME (query));

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
gst_audiosync_check_get_range (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "get_range");

  return FALSE;
}

static void
gst_audiosync_fixate (GstBaseSrc * src, GstCaps * caps)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);
  GstStructure *structure;

  GST_DEBUG_OBJECT (audiosync, "fixate");

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "rate", 48000);

}

static gboolean
gst_audiosync_unlock_stop (GstBaseSrc * src)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "unlock_stop");

  return TRUE;
}

static gboolean
gst_audiosync_prepare_seek_segment (GstBaseSrc * src, GstEvent * seek,
    GstSegment * segment)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);

  GST_DEBUG_OBJECT (audiosync, "seek_segment");

  return FALSE;
}

static GstFlowReturn
gst_audiosync_create (GstPushSrc * src, GstBuffer ** buf)
{
  GstAudiosync *audiosync = GST_AUDIOSYNC (src);
  GstBuffer *buffer;
  GstClockTime current_time;
  GstClockTime next_time;

  current_time = audiosync->timestamp_offset +
      gst_util_uint64_scale_int (audiosync->n_samples * GST_SECOND,
      1, audiosync->rate);
  next_time = audiosync->timestamp_offset +
      gst_util_uint64_scale_int ((audiosync->n_samples + 1024) * GST_SECOND,
      1, audiosync->rate);

  GST_DEBUG ("current_time %" GST_TIME_FORMAT, GST_TIME_ARGS (current_time));

  buffer = NULL;
  g_mutex_lock (audiosync->lock);
  if (audiosync->current_buffer) {
    GstClockTime buffer_start;
    GstClockTime buffer_end;

    buffer_start = GST_BUFFER_TIMESTAMP (audiosync->current_buffer) +
        audiosync->ts_offset;
    if (GST_BUFFER_DURATION (audiosync->current_buffer) != GST_CLOCK_TIME_NONE) {
      buffer_end =
          buffer_start + GST_BUFFER_DURATION (audiosync->current_buffer);
    } else {
      buffer_end = buffer_start;
    }

    if (current_time >= buffer_start && current_time <= buffer_end) {
      buffer = gst_buffer_ref (audiosync->current_buffer);
    }
  }
  audiosync->current_time = next_time;
  g_cond_signal (audiosync->cond);
  g_mutex_unlock (audiosync->lock);

  if (buffer == NULL) {
    buffer = gst_audiosync_create_silence (audiosync);
  }

  buffer = gst_buffer_make_metadata_writable (buffer);


  GST_BUFFER_TIMESTAMP (buffer) = current_time;
  GST_BUFFER_DURATION (buffer) = next_time - current_time;
  GST_BUFFER_OFFSET (buffer) = audiosync->n_samples;
  GST_BUFFER_OFFSET_END (buffer) = audiosync->n_samples + 1024;

  gst_buffer_set_caps (buffer, GST_PAD_CAPS (GST_BASE_SRC_PAD (audiosync)));

  audiosync->n_samples += 1024;

  *buf = buffer;

  return GST_FLOW_OK;
}

static GstCaps *
gst_audiosync_sink_getcaps (GstPad * pad)
{
  GstAudiosync *audiosync;
  GstCaps *peercaps;
  GstCaps *caps;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_ERROR_OBJECT (audiosync, "getcaps");

  peercaps = gst_pad_peer_get_caps_reffed (GST_BASE_SRC_PAD (audiosync));
  if (peercaps) {
    caps =
        gst_caps_intersect (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD
            (audiosync)), peercaps);
    gst_caps_unref (peercaps);
  } else {
    caps =
        gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD
            (audiosync)));
  }

  gst_object_unref (audiosync);
  return caps;
}

static gboolean
gst_audiosync_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstAudiosync *audiosync;
  gboolean ret;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_ERROR_OBJECT (audiosync, "setcaps");

  ret = gst_pad_set_caps (GST_BASE_SRC_PAD (audiosync), caps);

  gst_object_unref (audiosync);
  return ret;
}

static gboolean
gst_audiosync_sink_acceptcaps (GstPad * pad, GstCaps * caps)
{
  GstAudiosync *audiosync;
  gboolean ret;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "acceptcaps");

  ret = gst_pad_accept_caps (GST_BASE_SRC_PAD (audiosync), caps);

  gst_object_unref (audiosync);
  return ret;
}

static void
gst_audiosync_sink_fixatecaps (GstPad * pad, GstCaps * caps)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "fixatecaps");

  gst_object_unref (audiosync);
}

static gboolean
gst_audiosync_sink_activate (GstPad * pad)
{
  GstAudiosync *audiosync;
  gboolean ret;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "activate");

  if (gst_pad_check_pull_range (pad)) {
    GST_DEBUG_OBJECT (pad, "activating pull");
    ret = gst_pad_activate_pull (pad, TRUE);
  } else {
    GST_DEBUG_OBJECT (pad, "activating push");
    ret = gst_pad_activate_push (pad, TRUE);
  }

  gst_object_unref (audiosync);
  return ret;
}

static gboolean
gst_audiosync_sink_activatepush (GstPad * pad, gboolean active)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "activatepush %d", active);


  gst_object_unref (audiosync);
  return TRUE;
}

static gboolean
gst_audiosync_sink_activatepull (GstPad * pad, gboolean active)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "activatepull %d", active);


  gst_object_unref (audiosync);
  return TRUE;
}

static GstPadLinkReturn
gst_audiosync_sink_link (GstPad * pad, GstPad * peer)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "link");


  gst_object_unref (audiosync);
  return GST_PAD_LINK_OK;
}

static void
gst_audiosync_sink_unlink (GstPad * pad)
{
#if 0
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "unlink");


  gst_object_unref (audiosync);
#endif
}

static GstFlowReturn
gst_audiosync_sink_chain (GstPad * pad, GstBuffer * buffer)
{
  GstAudiosync *audiosync;
  GstClockTime buffer_start;
  GstClockTime buffer_end;
  GstFlowReturn ret = GST_FLOW_OK;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "chain");

  if (GST_BUFFER_IS_DISCONT (buffer)) {
    audiosync->ts_offset = audiosync->current_time -
        GST_BUFFER_TIMESTAMP (buffer);
    GST_ERROR ("setting ts offset to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (audiosync->ts_offset));
  }

  buffer_start = GST_BUFFER_TIMESTAMP (buffer) + audiosync->ts_offset;
  if (GST_BUFFER_DURATION (buffer) != GST_CLOCK_TIME_NONE) {
    buffer_end = buffer_start + GST_BUFFER_DURATION (buffer);
  } else {
    buffer_end = buffer_start;
  }

  g_mutex_lock (audiosync->lock);
  while (!audiosync->reset && buffer_start > audiosync->current_time) {
    GST_INFO ("buffer early, waiting (%" GST_TIME_FORMAT " > %" GST_TIME_FORMAT,
        GST_TIME_ARGS (buffer_start), GST_TIME_ARGS (audiosync->current_time));
    /* too early, wait. */
    g_cond_wait (audiosync->cond, audiosync->lock);
  }
  if (audiosync->reset) {
    GST_DEBUG ("got reset");
    ret = GST_FLOW_ERROR;
  }
  if (audiosync->current_buffer) {
    gst_buffer_unref (audiosync->current_buffer);
  }
  audiosync->current_buffer = buffer;
  g_mutex_unlock (audiosync->lock);

  gst_object_unref (audiosync);
  return ret;
}

static GstFlowReturn
gst_audiosync_sink_chainlist (GstPad * pad, GstBufferList * bufferlist)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "chainlist");


  gst_object_unref (audiosync);
  return GST_FLOW_OK;
}

static gboolean
gst_audiosync_sink_event (GstPad * pad, GstEvent * event)
{
  gboolean res;
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "event type %s", GST_EVENT_TYPE_NAME (event));

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
      gst_segment_set_newsegment_full (&audiosync->segment,
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

  gst_object_unref (audiosync);
  return res;
}

static gboolean
gst_audiosync_sink_query (GstPad * pad, GstQuery * query)
{
  gboolean res;
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "query");

  switch (GST_QUERY_TYPE (query)) {
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

  gst_object_unref (audiosync);
  return res;
}

static GstFlowReturn
gst_audiosync_sink_bufferalloc (GstPad * pad, guint64 offset, guint size,
    GstCaps * caps, GstBuffer ** buf)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "bufferalloc");


  *buf = gst_buffer_new_and_alloc (size);
  gst_buffer_set_caps (*buf, caps);

  gst_object_unref (audiosync);
  return GST_FLOW_OK;
}

static GstIterator *
gst_audiosync_sink_iterintlink (GstPad * pad)
{
  GstAudiosync *audiosync;
  GstIterator *iter;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "iterintlink");

  iter = gst_pad_iterate_internal_links_default (pad);

  gst_object_unref (audiosync);
  return iter;
}


static GstBuffer *
gst_audiosync_create_silence (GstAudiosync * audiosync)
{
  GstBuffer *buffer;
  guint8 *data;
  int size;

  size = 2 * 2 * 1024;

  buffer = gst_buffer_new_and_alloc (size);
  data = GST_BUFFER_DATA (buffer);

  memset (data, 0, size);

  return buffer;
}
