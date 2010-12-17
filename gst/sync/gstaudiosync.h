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

#ifndef _GST_AUDIOSYNC_H_
#define _GST_AUDIOSYNC_H_

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/audio/audio.h>

G_BEGIN_DECLS

#define GST_TYPE_AUDIOSYNC   (gst_audiosync_get_type())
#define GST_AUDIOSYNC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIOSYNC,GstAudiosync))
#define GST_AUDIOSYNC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AUDIOSYNC,GstAudiosyncClass))
#define GST_IS_AUDIOSYNC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIOSYNC))
#define GST_IS_AUDIOSYNC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AUDIOSYNC))

typedef struct _GstAudiosync GstAudiosync;
typedef struct _GstAudiosyncClass GstAudiosyncClass;

struct _GstAudiosync
{
  GstPushSrc base_audiosync;

  GstPad *sinkpad;
  GstPad *srcpad;

  GMutex *lock;
  GCond *cond;
  GstBuffer *current_buffer;
  GstSegment segment;

  gboolean reset;
  gint64 ts_offset;

  /* properties */
  gint64 timestamp_offset;

  /* state */
  int n_samples;
  int rate;
  int channels;

  GstClockTime current_time;
};

struct _GstAudiosyncClass
{
  GstPushSrcClass base_audiosync_class;
};

GType gst_audiosync_get_type (void);

G_END_DECLS

#endif
