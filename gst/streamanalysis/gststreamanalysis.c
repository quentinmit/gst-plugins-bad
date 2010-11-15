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
 * SECTION:element-gststreamanalysis
 *
 * The gststreamanalysis element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v fakesrc ! gststreamanalysis ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gststreamanalysis.h"

#include <string.h>

/* prototypes */


static void gst_stream_analysis_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_stream_analysis_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_stream_analysis_dispose (GObject * object);
static void gst_stream_analysis_finalize (GObject * object);

static gboolean gst_stream_analysis_start (GstBaseTransform * trans);
static gboolean gst_stream_analysis_stop (GstBaseTransform * trans);
static GstFlowReturn gst_stream_analysis_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);

enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_stream_analysis_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_stream_analysis_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* class initialization */

GST_BOILERPLATE (GstStreamAnalysis, gst_stream_analysis, GstBaseTransform,
    GST_TYPE_BASE_TRANSFORM);

static void
gst_stream_analysis_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_stream_analysis_src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_stream_analysis_sink_template));

  gst_element_class_set_details_simple (element_class, "FIXME",
      "Generic", "FIXME", "David Schleef <ds@schleef.org>");
}

static void
gst_stream_analysis_class_init (GstStreamAnalysisClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->set_property = gst_stream_analysis_set_property;
  gobject_class->get_property = gst_stream_analysis_get_property;
  gobject_class->dispose = gst_stream_analysis_dispose;
  gobject_class->finalize = gst_stream_analysis_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_stream_analysis_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_stream_analysis_stop);
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_stream_analysis_transform_ip);

}

static void
gst_stream_analysis_init (GstStreamAnalysis * streamanalysis,
    GstStreamAnalysisClass * streamanalysis_class)
{

}

void
gst_stream_analysis_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstStreamAnalysis *streamanalysis;

  g_return_if_fail (GST_IS_STREAM_ANALYSIS (object));
  streamanalysis = GST_STREAM_ANALYSIS (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_stream_analysis_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstStreamAnalysis *streamanalysis;

  g_return_if_fail (GST_IS_STREAM_ANALYSIS (object));
  streamanalysis = GST_STREAM_ANALYSIS (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_stream_analysis_dispose (GObject * object)
{
  GstStreamAnalysis *streamanalysis;

  g_return_if_fail (GST_IS_STREAM_ANALYSIS (object));
  streamanalysis = GST_STREAM_ANALYSIS (object);

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

void
gst_stream_analysis_finalize (GObject * object)
{
  GstStreamAnalysis *streamanalysis;

  g_return_if_fail (GST_IS_STREAM_ANALYSIS (object));
  streamanalysis = GST_STREAM_ANALYSIS (object);

  /* clean up object here */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gboolean
gst_stream_analysis_start (GstBaseTransform * trans)
{
  GstStreamAnalysis *sa = GST_STREAM_ANALYSIS (trans);

  memset (sa->sizes, 0, sizeof (int) * N_SIZES);

  return TRUE;
}

static gboolean
gst_stream_analysis_stop (GstBaseTransform * trans)
{

  return TRUE;
}

static GstFlowReturn
gst_stream_analysis_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstStreamAnalysis *sa = GST_STREAM_ANALYSIS (trans);
  int i;
  int sum25;
  int sum50;
  int sum250;

  memmove (sa->sizes + 1, sa->sizes, sizeof (int) * (N_SIZES - 1));
  sa->sizes[0] = GST_BUFFER_SIZE (buf);

  sum25 = 0;
  for (i = 0; i < 25; i++) {
    sum25 += sa->sizes[i];
  }
  sum50 = sum25;
  for (i = 25; i < 50; i++) {
    sum50 += sa->sizes[i];
  }
  sum250 = sum50;
  for (i = 50; i < 250; i++) {
    sum250 += sa->sizes[i];
  }

  g_print ("%d %d %d %d\n", sa->sizes[0] * 8, sum25 * 8, sum50 * 8, sum250 * 8);

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  gst_element_register (plugin, "streamanalysis", GST_RANK_NONE,
      gst_stream_analysis_get_type ());

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "streamanalysis",
    "FIXME", plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
