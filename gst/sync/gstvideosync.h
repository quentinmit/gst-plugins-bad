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

#ifndef _GST_VIDEOSYNC_H_
#define _GST_VIDEOSYNC_H_

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEOSYNC   (gst_videosync_get_type())
#define GST_VIDEOSYNC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEOSYNC,GstVideosync))
#define GST_VIDEOSYNC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEOSYNC,GstVideosyncClass))
#define GST_IS_VIDEOSYNC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEOSYNC))
#define GST_IS_VIDEOSYNC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEOSYNC))

typedef struct _GstVideosync GstVideosync;
typedef struct _GstVideosyncClass GstVideosyncClass;

struct _GstVideosync
{
  GstPushSrc base_videosync;

  GstPad *sinkpad;
  GstPad *srcpad;

  GMutex *lock;
  GCond *cond;
  GstBuffer *current_buffer;
  GstSegment sink_segment;
  GstSegment src_segment;

  gboolean reset;
  gint64 ts_offset;

  /* properties */
  gint64 timestamp_offset;

  /* state */
  int width;
  int height;
  GstVideoFormat format;
  int size;
  int fps_n, fps_d;
  int par_n, par_d;
  gboolean interlaced;

  int n_frames;

  GstClockTime current_time;
  GstClockTime buffer_start;
  GstClockTime buffer_end;
};

struct _GstVideosyncClass
{
  GstPushSrcClass base_videosync_class;
};

GType gst_videosync_get_type (void);

G_END_DECLS

#endif
