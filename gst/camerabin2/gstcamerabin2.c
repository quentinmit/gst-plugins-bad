/* GStreamer
 * Copyright (C) 2010 Thiago Santos <thiago.sousa.santos@collabora.co.uk>
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
 * SECTION:element-gstcamerabin2
 *
 * The gstcamerabin2 element does FIXME stuff.
 *
 * Note that camerabin2 is still UNSTABLE, EXPERIMENTAL and under heavy
 * development.
 */

/*
 * Detail Topics:
 *
 * videorecordingbin state management (for now on vidbin)
 * - The problem: keeping vidbin state in sync with camerabin will make it
 *                go to playing when it might not be used, causing its internal
 *                filesink to open a file that might be left blank.
 * - The solution: vidbin state is set to locked upon its creation and camerabin
 *                 registers itself on the notify::ready-for-capture of the src.
 *                 Whenever the src readyness goes to FALSE it means a new
 *                 capture is starting. If we are on video mode, the vidbin's
 *                 state is set to NULL and then PLAYING (in between this we
 *                 have room to set the destination filename).
 *                 There is no problem to leave it on playing after an EOS, so
 *                 no action is taken on stop-capture.
 * - TODO: What happens when an error pops?
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstcamerabin2.h"
#include "gstcamerabin-enum.h"

GST_DEBUG_CATEGORY_STATIC (gst_camera_bin_debug);
#define GST_CAT_DEFAULT gst_camera_bin_debug

/* prototypes */

enum
{
  PROP_0,
  PROP_MODE,
  PROP_LOCATION
};

enum
{
  /* action signals */
  START_CAPTURE_SIGNAL,
  STOP_CAPTURE_SIGNAL,
  /* emit signals */
  LAST_SIGNAL
};
static guint camerabin_signals[LAST_SIGNAL];

#define DEFAULT_MODE MODE_IMAGE
#define DEFAULT_VID_LOCATION "vid_%d"
#define DEFAULT_IMG_LOCATION "img_%d"

/********************************
 * Standard GObject boilerplate *
 * and GObject types            *
 ********************************/

static GstPipelineClass *parent_class;
static void gst_camera_bin_class_init (GstCameraBinClass * klass);
static void gst_camera_bin_base_init (gpointer klass);
static void gst_camera_bin_init (GstCameraBin * camera);
static void gst_camera_bin_dispose (GObject * object);
static void gst_camera_bin_finalize (GObject * object);

GType
gst_camera_bin_get_type (void)
{
  static GType gst_camera_bin_type = 0;

  if (!gst_camera_bin_type) {
    static const GTypeInfo gst_camera_bin_info = {
      sizeof (GstCameraBinClass),
      (GBaseInitFunc) gst_camera_bin_base_init,
      NULL,
      (GClassInitFunc) gst_camera_bin_class_init,
      NULL,
      NULL,
      sizeof (GstCameraBin),
      0,
      (GInstanceInitFunc) gst_camera_bin_init,
      NULL
    };

    gst_camera_bin_type =
        g_type_register_static (GST_TYPE_PIPELINE, "GstCameraBin2",
        &gst_camera_bin_info, 0);
  }

  return gst_camera_bin_type;
}

/* GObject class functions */
static void gst_camera_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_camera_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/* Element class functions */
static GstStateChangeReturn
gst_camera_bin_change_state (GstElement * element, GstStateChange trans);


/* Camerabin functions */

static void
gst_camera_bin_start_capture (GstCameraBin * camerabin)
{
  GST_DEBUG_OBJECT (camerabin, "Received start-capture");
  g_signal_emit_by_name (camerabin->src, "start-capture", NULL);
}

static void
gst_camera_bin_stop_capture (GstCameraBin * camerabin)
{
  g_signal_emit_by_name (camerabin->src, "stop-capture", NULL);
}

static void
gst_camera_bin_change_mode (GstCameraBin * camerabin, gint mode)
{
  if (mode == camerabin->mode)
    return;

  /* stop any ongoing capture */
  gst_camera_bin_stop_capture (camerabin);
  camerabin->mode = mode;
  g_object_set (camerabin->src, "mode", mode, NULL);
}

static void
gst_camera_bin_src_notify_readyforcapture (GObject * obj, GParamSpec * pspec,
    gpointer user_data)
{
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (user_data);
  gboolean ready;

  if (camera->mode == MODE_VIDEO) {
    g_object_get (camera->src, "ready-for-capture", &ready, NULL);
    if (!ready) {
      gchar *location;

      /* a video recording is about to start, we reset the videobin */
      gst_element_set_state (camera->vidbin, GST_STATE_NULL);
      location = g_strdup_printf (camera->vid_location, camera->vid_index++);
      g_object_set (camera->vidbin, "location", location, NULL);
      g_free (location);
      gst_element_set_state (camera->vidbin, GST_STATE_PLAYING);
    }
  }
}

static void
gst_camera_bin_dispose (GObject * object)
{
  GstCameraBin *camerabin = GST_CAMERA_BIN_CAST (object);

  g_free (camerabin->img_location);
  g_free (camerabin->vid_location);

  if (camerabin->src_capture_notify_id)
    g_signal_handler_disconnect (camerabin->src,
        camerabin->src_capture_notify_id);
  if (camerabin->src)
    gst_object_unref (camerabin->src);

  if (camerabin->vidbin)
    gst_object_unref (camerabin->vidbin);

  if (camerabin->imgbin)
    gst_object_unref (camerabin->imgbin);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_camera_bin_finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_camera_bin_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class, "CameraBin2",
      "Generic/Bin/Camera", "CameraBin2",
      "Thiago Santos <thiago.sousa.santos@collabora.co.uk>");
}

static void
gst_camera_bin_class_init (GstCameraBinClass * klass)
{
  GObjectClass *object_class;
  GstElementClass *element_class;

  parent_class = g_type_class_peek_parent (klass);
  object_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);

  object_class->dispose = gst_camera_bin_dispose;
  object_class->finalize = gst_camera_bin_finalize;
  object_class->set_property = gst_camera_bin_set_property;
  object_class->get_property = gst_camera_bin_get_property;

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_camera_bin_change_state);

  klass->start_capture = gst_camera_bin_start_capture;
  klass->stop_capture = gst_camera_bin_stop_capture;

  /**
   * GstCameraBin:mode:
   *
   * Set the mode of operation: still image capturing or video recording.
   */
  g_object_class_install_property (object_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode",
          "The capture mode (still image capture or video recording)",
          GST_TYPE_CAMERABIN_MODE, DEFAULT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location",
          "Location to save the captured files. A %d might be used on the"
          "filename as a placeholder for a numeric index of the capture."
          "Default for images is img_%d and vid_%d for videos",
          DEFAULT_IMG_LOCATION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstCameraBin::capture-start:
   * @camera: the camera bin element
   *
   * Starts image capture or video recording depending on the Mode.
   */
  camerabin_signals[START_CAPTURE_SIGNAL] =
      g_signal_new ("start-capture",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstCameraBinClass, start_capture),
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * GstCameraBin::capture-stop:
   * @camera: the camera bin element
   */
  camerabin_signals[STOP_CAPTURE_SIGNAL] =
      g_signal_new ("stop-capture",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstCameraBinClass, stop_capture),
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
gst_camera_bin_init (GstCameraBin * camerabin)
{
  camerabin->mode = DEFAULT_MODE;
  camerabin->vid_location = g_strdup (DEFAULT_VID_LOCATION);
  camerabin->img_location = g_strdup (DEFAULT_IMG_LOCATION);
}

/**
 * gst_camera_bin_create_elements:
 * @param camera: the #GstCameraBin
 *
 * Creates all elements inside #GstCameraBin
 *
 * Each of the pads on the camera source is linked as follows:
 * .pad ! queue ! capsfilter ! correspondingbin
 *
 * Where 'correspondingbin' is the bin appropriate for
 * the camera source pad.
 */
static gboolean
gst_camera_bin_create_elements (GstCameraBin * camera)
{
  GstElement *src;
  GstElement *vid;
  GstElement *img;
  GstElement *vf;
  GstElement *vid_queue;
  GstElement *img_queue;
  GstElement *vf_queue;
  GstElement *vid_capsfilter;
  GstElement *img_capsfilter;
  GstElement *vf_capsfilter;

  if (camera->elements_created)
    return TRUE;

  src = gst_element_factory_make ("v4l2camerasrc", "camerasrc");
  vid = gst_element_factory_make ("videorecordingbin", "video-rec-bin");
  img = gst_element_factory_make ("imagecapturebin", "image-cap-bin");
  vf = gst_element_factory_make ("viewfinderbin", "vf-bin");

  camera->src = gst_object_ref (src);
  camera->vidbin = gst_object_ref (vid);
  camera->imgbin = gst_object_ref (img);

  vid_queue = gst_element_factory_make ("queue", "video-queue");
  img_queue = gst_element_factory_make ("queue", "image-queue");
  vf_queue = gst_element_factory_make ("queue", "vf-queue");

  vid_capsfilter = gst_element_factory_make ("capsfilter", "video-capsfilter");
  img_capsfilter = gst_element_factory_make ("capsfilter", "image-capsfilter");
  vf_capsfilter = gst_element_factory_make ("capsfilter", "vf-capsfilter");

  gst_bin_add_many (GST_BIN_CAST (camera), src, vid, img, vf, vid_queue,
      img_queue, vf_queue, vid_capsfilter, img_capsfilter, vf_capsfilter, NULL);

  /* Linking can be optimized TODO */
  gst_element_link_many (vid_queue, vid_capsfilter, vid, NULL);
  gst_element_link_many (img_queue, img_capsfilter, img, NULL);
  gst_element_link_many (vf_queue, vf_capsfilter, vf, NULL);
  gst_element_link_pads (src, "vfsrc", vf_queue, "sink");
  gst_element_link_pads (src, "imgsrc", img_queue, "sink");
  gst_element_link_pads (src, "vidsrc", vid_queue, "sink");

  /*
   * Video can't get into playing as its internal filesink will open
   * a file for writing and leave it empty if unused.
   *
   * Its state is managed using the current mode and the source's
   * ready-for-capture notify callback. When we are at video mode and
   * the source's ready-for-capture goes to FALSE it means it is
   * starting recording, so we should prepare the video bin.
   */
  gst_element_set_locked_state (vid, TRUE);
  camera->src_capture_notify_id = g_signal_connect (G_OBJECT (src),
      "notify::ready-for-capture",
      G_CALLBACK (gst_camera_bin_src_notify_readyforcapture), camera);

  g_object_set (vid, "location", camera->vid_location, NULL);
  g_object_set (img, "location", camera->img_location, NULL);

  camera->elements_created = TRUE;
  return TRUE;
}

static GstStateChangeReturn
gst_camera_bin_change_state (GstElement * element, GstStateChange trans)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (element);

  switch (trans) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_camera_bin_create_elements (camera)) {
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, trans);

  switch (trans) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_camera_bin_set_location (GstCameraBin * camera, const gchar * location)
{
  if (camera->mode == MODE_IMAGE) {
    g_object_set (camera->imgbin, "location", location, NULL);
    g_free (camera->img_location);
    camera->img_location = g_strdup (location);
  } else {
    g_free (camera->vid_location);
    camera->vid_location = g_strdup (location);
  }
}

static void
gst_camera_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (object);

  switch (prop_id) {
    case PROP_MODE:
      gst_camera_bin_change_mode (camera, g_value_get_enum (value));
      break;
    case PROP_LOCATION:
      gst_camera_bin_set_location (camera, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_camera_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (object);

  switch (prop_id) {
    case PROP_MODE:
      g_value_set_enum (value, camera->mode);
      break;
    case PROP_LOCATION:
      if (camera->mode == MODE_VIDEO) {
        g_value_set_string (value, camera->vid_location);
      } else {
        g_value_set_string (value, camera->img_location);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_camera_bin_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_camera_bin_debug, "camerabin2", 0, "CameraBin2");

  return gst_element_register (plugin, "camerabin2", GST_RANK_NONE,
      gst_camera_bin_get_type ());
}