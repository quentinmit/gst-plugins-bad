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
#include <gst/gst.h>
#include "gstaudiosync.h"

/* prototypes */


static void gst_audiosync_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_audiosync_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_audiosync_dispose (GObject * object);
static void gst_audiosync_finalize (GObject * object);

static GstPad *gst_audiosync_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name);
static void gst_audiosync_release_pad (GstElement * element, GstPad * pad);
static GstStateChangeReturn
gst_audiosync_change_state (GstElement * element, GstStateChange transition);
static GstClock *gst_audiosync_provide_clock (GstElement * element);
static gboolean gst_audiosync_set_clock (GstElement * element,
    GstClock * clock);
static GstIndex *gst_audiosync_get_index (GstElement * element);
static void gst_audiosync_set_index (GstElement * element, GstIndex * index);
static gboolean gst_audiosync_send_event (GstElement * element,
    GstEvent * event);
static gboolean gst_audiosync_query (GstElement * element, GstQuery * query);

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


static GstCaps *gst_audiosync_src_getcaps (GstPad * pad);
static gboolean gst_audiosync_src_setcaps (GstPad * pad, GstCaps * caps);
static gboolean gst_audiosync_src_acceptcaps (GstPad * pad, GstCaps * caps);
static void gst_audiosync_src_fixatecaps (GstPad * pad, GstCaps * caps);
static gboolean gst_audiosync_src_activate (GstPad * pad);
static gboolean gst_audiosync_src_activatepush (GstPad * pad, gboolean active);
static gboolean gst_audiosync_src_activatepull (GstPad * pad, gboolean active);
static GstPadLinkReturn gst_audiosync_src_link (GstPad * pad, GstPad * peer);
static void gst_audiosync_src_unlink (GstPad * pad);
static GstFlowReturn gst_audiosync_src_getrange (GstPad * pad, guint64 offset,
    guint length, GstBuffer ** buffer);
static gboolean gst_audiosync_src_event (GstPad * pad, GstEvent * event);
static gboolean gst_audiosync_src_query (GstPad * pad, GstQuery * query);
static GstIterator *gst_audiosync_src_iterintlink (GstPad * pad);


enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_audiosync_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );

static GstStaticPadTemplate gst_audiosync_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );


/* class initialization */

GST_BOILERPLATE (GstAudiosync, gst_audiosync, GstElement, GST_TYPE_ELEMENT);

static void
gst_audiosync_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

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
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_audiosync_set_property;
  gobject_class->get_property = gst_audiosync_get_property;
  gobject_class->dispose = gst_audiosync_dispose;
  gobject_class->finalize = gst_audiosync_finalize;
  element_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_audiosync_request_new_pad);
  element_class->release_pad = GST_DEBUG_FUNCPTR (gst_audiosync_release_pad);
  element_class->change_state = GST_DEBUG_FUNCPTR (gst_audiosync_change_state);
  element_class->provide_clock =
      GST_DEBUG_FUNCPTR (gst_audiosync_provide_clock);
  element_class->set_clock = GST_DEBUG_FUNCPTR (gst_audiosync_set_clock);
  element_class->get_index = GST_DEBUG_FUNCPTR (gst_audiosync_get_index);
  element_class->set_index = GST_DEBUG_FUNCPTR (gst_audiosync_set_index);
  element_class->send_event = GST_DEBUG_FUNCPTR (gst_audiosync_send_event);
  element_class->query = GST_DEBUG_FUNCPTR (gst_audiosync_query);

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
  gst_pad_set_bufferalloc_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_bufferalloc));
  gst_pad_set_iterate_internal_links_function (audiosync->sinkpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_sink_iterintlink));
  gst_element_add_pad (GST_ELEMENT (audiosync), audiosync->sinkpad);



  audiosync->srcpad =
      gst_pad_new_from_static_template (&gst_audiosync_src_template, "src");
  gst_pad_set_getcaps_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_getcaps));
  gst_pad_set_setcaps_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_setcaps));
  gst_pad_set_acceptcaps_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_acceptcaps));
  gst_pad_set_fixatecaps_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_fixatecaps));
  gst_pad_set_activate_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_activate));
  gst_pad_set_activatepush_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_activatepush));
  gst_pad_set_activatepull_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_activatepull));
  gst_pad_set_link_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_link));
  gst_pad_set_unlink_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_unlink));
  gst_pad_set_getrange_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_getrange));
  gst_pad_set_event_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_event));
  gst_pad_set_query_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_query));
  gst_pad_set_iterate_internal_links_function (audiosync->srcpad,
      GST_DEBUG_FUNCPTR (gst_audiosync_src_iterintlink));
  gst_element_add_pad (GST_ELEMENT (audiosync), audiosync->srcpad);


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



static GstPad *
gst_audiosync_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name)
{

  return NULL;
}

static void
gst_audiosync_release_pad (GstElement * element, GstPad * pad)
{

}

static GstStateChangeReturn
gst_audiosync_change_state (GstElement * element, GstStateChange transition)
{
  GstAudiosync *audiosync;
  GstStateChangeReturn ret;

  g_return_val_if_fail (GST_IS_AUDIOSYNC (element), GST_STATE_CHANGE_FAILURE);
  audiosync = GST_AUDIOSYNC (element);

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
gst_audiosync_provide_clock (GstElement * element)
{

  return NULL;
}

static gboolean
gst_audiosync_set_clock (GstElement * element, GstClock * clock)
{

  return TRUE;
}

static GstIndex *
gst_audiosync_get_index (GstElement * element)
{

  return NULL;
}

static void
gst_audiosync_set_index (GstElement * element, GstIndex * index)
{

}

static gboolean
gst_audiosync_send_event (GstElement * element, GstEvent * event)
{

  return TRUE;
}

static gboolean
gst_audiosync_query (GstElement * element, GstQuery * query)
{

  return FALSE;
}

static GstCaps *
gst_audiosync_sink_getcaps (GstPad * pad)
{
  GstAudiosync *audiosync;
  GstCaps *caps;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "getcaps");

  caps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));

  gst_object_unref (audiosync);
  return caps;
}

static gboolean
gst_audiosync_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "setcaps");


  gst_object_unref (audiosync);
  return TRUE;
}

static gboolean
gst_audiosync_sink_acceptcaps (GstPad * pad, GstCaps * caps)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "acceptcaps");


  gst_object_unref (audiosync);
  return TRUE;
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

  GST_DEBUG_OBJECT (audiosync, "activatepush");


  gst_object_unref (audiosync);
  return TRUE;
}

static gboolean
gst_audiosync_sink_activatepull (GstPad * pad, gboolean active)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "activatepull");


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
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "unlink");


  gst_object_unref (audiosync);
}

static GstFlowReturn
gst_audiosync_sink_chain (GstPad * pad, GstBuffer * buffer)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "chain");


  gst_object_unref (audiosync);
  return GST_FLOW_OK;
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

  GST_DEBUG_OBJECT (audiosync, "event");

  switch (GST_EVENT_TYPE (event)) {
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


static GstCaps *
gst_audiosync_src_getcaps (GstPad * pad)
{
  GstAudiosync *audiosync;
  GstCaps *caps;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "getcaps");

  caps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));

  gst_object_unref (audiosync);
  return caps;
}

static gboolean
gst_audiosync_src_setcaps (GstPad * pad, GstCaps * caps)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "setcaps");


  gst_object_unref (audiosync);
  return TRUE;
}

static gboolean
gst_audiosync_src_acceptcaps (GstPad * pad, GstCaps * caps)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "acceptcaps");


  gst_object_unref (audiosync);
  return TRUE;
}

static void
gst_audiosync_src_fixatecaps (GstPad * pad, GstCaps * caps)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "fixatecaps");


  gst_object_unref (audiosync);
}

static gboolean
gst_audiosync_src_activate (GstPad * pad)
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
gst_audiosync_src_activatepush (GstPad * pad, gboolean active)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "activatepush");


  gst_object_unref (audiosync);
  return TRUE;
}

static gboolean
gst_audiosync_src_activatepull (GstPad * pad, gboolean active)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "activatepull");


  gst_object_unref (audiosync);
  return TRUE;
}

static GstPadLinkReturn
gst_audiosync_src_link (GstPad * pad, GstPad * peer)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "link");


  gst_object_unref (audiosync);
  return GST_PAD_LINK_OK;
}

static void
gst_audiosync_src_unlink (GstPad * pad)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "unlink");


  gst_object_unref (audiosync);
}

static GstFlowReturn
gst_audiosync_src_getrange (GstPad * pad, guint64 offset, guint length,
    GstBuffer ** buffer)
{
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "getrange");


  gst_object_unref (audiosync);
  return GST_FLOW_OK;
}

static gboolean
gst_audiosync_src_event (GstPad * pad, GstEvent * event)
{
  gboolean res;
  GstAudiosync *audiosync;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "event");

  switch (GST_EVENT_TYPE (event)) {
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (audiosync);
  return res;
}

static gboolean
gst_audiosync_src_query (GstPad * pad, GstQuery * query)
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

static GstIterator *
gst_audiosync_src_iterintlink (GstPad * pad)
{
  GstAudiosync *audiosync;
  GstIterator *iter;

  audiosync = GST_AUDIOSYNC (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT (audiosync, "iterintlink");

  iter = gst_pad_iterate_internal_links_default (pad);

  gst_object_unref (audiosync);
  return iter;
}
