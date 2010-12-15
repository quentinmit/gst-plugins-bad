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
#include <gst/gst.h>
#include "gstvideosync.h"

/* prototypes */


static void gst_videosync_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_videosync_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_videosync_dispose (GObject * object);
static void gst_videosync_finalize (GObject * object);

static GstPad *gst_videosync_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name);
static void gst_videosync_release_pad (GstElement * element, GstPad * pad);
static GstStateChangeReturn
gst_videosync_change_state (GstElement * element, GstStateChange transition);
static GstClock *gst_videosync_provide_clock (GstElement * element);
static gboolean gst_videosync_set_clock (GstElement * element,
    GstClock * clock);
static GstIndex *gst_videosync_get_index (GstElement * element);
static void gst_videosync_set_index (GstElement * element, GstIndex * index);
static gboolean gst_videosync_send_event (GstElement * element,
    GstEvent * event);
static gboolean gst_videosync_query (GstElement * element, GstQuery * query);

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


static GstCaps *gst_videosync_src_getcaps (GstPad * pad);
static gboolean gst_videosync_src_setcaps (GstPad * pad, GstCaps * caps);
static gboolean gst_videosync_src_acceptcaps (GstPad * pad, GstCaps * caps);
static void gst_videosync_src_fixatecaps (GstPad * pad, GstCaps * caps);
static gboolean gst_videosync_src_activate (GstPad * pad);
static gboolean gst_videosync_src_activatepush (GstPad * pad, gboolean active);
static gboolean gst_videosync_src_activatepull (GstPad * pad, gboolean active);
static GstPadLinkReturn gst_videosync_src_link (GstPad * pad, GstPad * peer);
static void gst_videosync_src_unlink (GstPad * pad);
static GstFlowReturn gst_videosync_src_getrange (GstPad * pad, guint64 offset,
    guint length, GstBuffer ** buffer);
static gboolean gst_videosync_src_event (GstPad * pad, GstEvent * event);
static gboolean gst_videosync_src_query (GstPad * pad, GstQuery * query);
static GstIterator *gst_videosync_src_iterintlink (GstPad * pad);


enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_videosync_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );

static GstStaticPadTemplate gst_videosync_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );


/* class initialization */

GST_BOILERPLATE (GstVideosync, gst_videosync, GstElement, GST_TYPE_ELEMENT);

static void
gst_videosync_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

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
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_videosync_set_property;
  gobject_class->get_property = gst_videosync_get_property;
  gobject_class->dispose = gst_videosync_dispose;
  gobject_class->finalize = gst_videosync_finalize;
  element_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_videosync_request_new_pad);
  element_class->release_pad = GST_DEBUG_FUNCPTR (gst_videosync_release_pad);
  element_class->change_state = GST_DEBUG_FUNCPTR (gst_videosync_change_state);
  element_class->provide_clock =
      GST_DEBUG_FUNCPTR (gst_videosync_provide_clock);
  element_class->set_clock = GST_DEBUG_FUNCPTR (gst_videosync_set_clock);
  element_class->get_index = GST_DEBUG_FUNCPTR (gst_videosync_get_index);
  element_class->set_index = GST_DEBUG_FUNCPTR (gst_videosync_set_index);
  element_class->send_event = GST_DEBUG_FUNCPTR (gst_videosync_send_event);
  element_class->query = GST_DEBUG_FUNCPTR (gst_videosync_query);

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
  gst_pad_set_bufferalloc_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_bufferalloc));
  gst_pad_set_iterate_internal_links_function (videosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_videosync_sink_iterintlink));
  gst_element_add_pad (GST_ELEMENT (videosync), videosync->sinkpad);



  videosync->srcpad =
      gst_pad_new_from_static_template (&gst_videosync_src_template, "src");
  gst_pad_set_getcaps_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_getcaps));
  gst_pad_set_setcaps_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_setcaps));
  gst_pad_set_acceptcaps_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_acceptcaps));
  gst_pad_set_fixatecaps_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_fixatecaps));
  gst_pad_set_activate_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_activate));
  gst_pad_set_activatepush_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_activatepush));
  gst_pad_set_activatepull_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_activatepull));
  gst_pad_set_link_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_link));
  gst_pad_set_unlink_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_unlink));
  gst_pad_set_getrange_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_getrange));
  gst_pad_set_event_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_event));
  gst_pad_set_query_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_query));
  gst_pad_set_iterate_internal_links_function (videosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_videosync_src_iterintlink));
  gst_element_add_pad (GST_ELEMENT (videosync), videosync->srcpad);


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



static GstPad *
gst_videosync_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name)
{

  return NULL;
}

static void
gst_videosync_release_pad (GstElement * element, GstPad * pad)
{

}

static GstStateChangeReturn
gst_videosync_change_state (GstElement * element, GstStateChange transition)
{
  GstVideosync *videosync;
  GstStateChangeReturn ret;

  g_return_val_if_fail (GST_IS_VIDEOSYNC (element), GST_STATE_CHANGE_FAILURE);
  videosync = GST_VIDEOSYNC (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static GstClock *
gst_videosync_provide_clock (GstElement * element)
{

  return NULL;
}

static gboolean
gst_videosync_set_clock (GstElement * element, GstClock * clock)
{

  return TRUE;
}

static GstIndex *
gst_videosync_get_index (GstElement * element)
{

  return NULL;
}

static void
gst_videosync_set_index (GstElement * element, GstIndex * index)
{

}

static gboolean
gst_videosync_send_event (GstElement * element, GstEvent * event)
{

  return TRUE;
}

static gboolean
gst_videosync_query (GstElement * element, GstQuery * query)
{

  return FALSE;
}

static GstCaps *
gst_videosync_sink_getcaps (GstPad * pad)
{
  GstVideosync *videosync;
  GstCaps *caps;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "getcaps");

  caps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));

  gst_object_unref (videosync);
  return caps;
}

static gboolean
gst_videosync_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "setcaps");


  gst_object_unref (videosync);
  return TRUE;
}

static gboolean
gst_videosync_sink_acceptcaps (GstPad * pad, GstCaps * caps)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "acceptcaps");


  gst_object_unref (videosync);
  return TRUE;
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

  GST_DEBUG_OBJECT (videosync, "activatepush");


  gst_object_unref (videosync);
  return TRUE;
}

static gboolean
gst_videosync_sink_activatepull (GstPad * pad, gboolean active)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "activatepull");


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
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "unlink");


  gst_object_unref (videosync);
}

static GstFlowReturn
gst_videosync_sink_chain (GstPad * pad, GstBuffer * buffer)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "chain");


  gst_object_unref (videosync);
  return GST_FLOW_OK;
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

  GST_DEBUG_OBJECT (videosync, "event");

  switch (GST_EVENT_TYPE (event)) {
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


static GstCaps *
gst_videosync_src_getcaps (GstPad * pad)
{
  GstVideosync *videosync;
  GstCaps *caps;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "getcaps");

  caps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));

  gst_object_unref (videosync);
  return caps;
}

static gboolean
gst_videosync_src_setcaps (GstPad * pad, GstCaps * caps)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "setcaps");


  gst_object_unref (videosync);
  return TRUE;
}

static gboolean
gst_videosync_src_acceptcaps (GstPad * pad, GstCaps * caps)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "acceptcaps");


  gst_object_unref (videosync);
  return TRUE;
}

static void
gst_videosync_src_fixatecaps (GstPad * pad, GstCaps * caps)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "fixatecaps");


  gst_object_unref (videosync);
}

static gboolean
gst_videosync_src_activate (GstPad * pad)
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
gst_videosync_src_activatepush (GstPad * pad, gboolean active)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "activatepush");


  gst_object_unref (videosync);
  return TRUE;
}

static gboolean
gst_videosync_src_activatepull (GstPad * pad, gboolean active)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "activatepull");


  gst_object_unref (videosync);
  return TRUE;
}

static GstPadLinkReturn
gst_videosync_src_link (GstPad * pad, GstPad * peer)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "link");


  gst_object_unref (videosync);
  return GST_PAD_LINK_OK;
}

static void
gst_videosync_src_unlink (GstPad * pad)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "unlink");


  gst_object_unref (videosync);
}

static GstFlowReturn
gst_videosync_src_getrange (GstPad * pad, guint64 offset, guint length,
    GstBuffer ** buffer)
{
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "getrange");


  gst_object_unref (videosync);
  return GST_FLOW_OK;
}

static gboolean
gst_videosync_src_event (GstPad * pad, GstEvent * event)
{
  gboolean res;
  GstVideosync *videosync;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "event");

  switch (GST_EVENT_TYPE (event)) {
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (videosync);
  return res;
}

static gboolean
gst_videosync_src_query (GstPad * pad, GstQuery * query)
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

static GstIterator *
gst_videosync_src_iterintlink (GstPad * pad)
{
  GstVideosync *videosync;
  GstIterator *iter;

  videosync = GST_VIDEOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (videosync, "iterintlink");

  iter = gst_pad_iterate_internal_links_default (pad);

  gst_object_unref (videosync);
  return iter;
}
