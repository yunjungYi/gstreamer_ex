#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include "gstnvdsmeta.h"

#define MAX_DISPLAY_LEN 64

#define PGIE_CLASS_ID_VEHICLE 0
#define PGIE_CLASS_ID_PERSON 2

/* The muxer output resolution must be set if the input streams will be of
 * different resolution. The muxer will scale all the input frames to this
 * resolution. */
#define MUXER_OUTPUT_WIDTH 1920
#define MUXER_OUTPUT_HEIGHT 1080

/* Muxer batch formation timeout, for e.g. 40 millisec. Should ideally be set
 * based on the fastest source's framerate. */
#define MUXER_BATCH_TIMEOUT_USEC 40000
GMainLoop *loop = NULL;
GstElement *pipeline = NULL;
gint frame_number = 0;
gchar pgie_classes_str[4][32] = { "Vehicle", "TwoWheeler", "Person",
  "Roadsign"
};

flag = TRUE;

static gboolean source_switch_thread(gpointer* data)
{
   g_print("debug \n");
GstElement* source = gst_bin_get_by_name(GST_BIN(pipeline), "file-source");
GstElement* h264parser = gst_bin_get_by_name(GST_BIN(pipeline), "h264-parser");
GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "nvvideo-renderer");
gst_element_set_state(pipeline, GST_STATE_PAUSED);
GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;
ret = gst_element_set_state(source, GST_STATE_NULL);
if (ret == GST_STATE_CHANGE_FAILURE)
{
g_print("Unable to set state change for source element \n");
g_main_loop_quit(loop);
}
if (flag)
{
g_object_set(G_OBJECT(source), "location", "./sample_720p_2.h264", NULL);
flag = FALSE;
}
else
{
g_object_set(G_OBJECT(source), "location", "./sample_720p.h264", NULL);
flag = TRUE;
}
gst_pad_activate_mode(gst_element_get_static_pad(h264parser, "sink"), GST_PAD_MODE_PUSH, TRUE);
gst_element_sync_state_with_parent(h264parser);
gst_element_sync_state_with_parent(source);
gst_element_sync_state_with_parent(sink);

frame_number = 0;
gst_element_set_state(pipeline, GST_STATE_PLAYING);
return FALSE;
}

static GstPadProbeReturn eos_probe_cb(GstPad* pad, GstPadProbeInfo* info, gpointer u_data)
{
if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) != GST_EVENT_EOS) { return GST_PAD_PROBE_OK; }

gst_element_seek((GstElement*) u_data, 1.0, GST_FORMAT_TIME,
             (GstSeekFlags)(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE,
             GST_CLOCK_TIME_NONE);

g_idle_add((GSourceFunc) source_switch_thread, u_data);
return GST_PAD_PROBE_DROP;
}


/* osd_sink_pad_buffer_probe  will extract metadata received on OSD sink pad
 * and update params for drawing rectangle, object information etc. */

static GstPadProbeReturn
osd_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info,
gpointer u_data)
{
GstBuffer *buf = (GstBuffer *) info->data;
guint num_rects = 0; 
NvDsObjectMeta *obj_meta = NULL;
guint vehicle_count = 0;
guint person_count = 0;
NvDsMetaList * l_frame = NULL;
NvDsMetaList * l_obj = NULL;
NvDsDisplayMeta *display_meta = NULL;

NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);

for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
  l_frame = l_frame->next) {
NvDsFrameMeta *frame_meta = (NvDsFrameMeta *) (l_frame->data);
int offset = 0;
for (l_obj = frame_meta->obj_meta_list; l_obj != NULL;
        l_obj = l_obj->next) {
    obj_meta = (NvDsObjectMeta *) (l_obj->data);
    if (obj_meta->class_id == PGIE_CLASS_ID_VEHICLE) {
        vehicle_count++;
        num_rects++;
    }
    if (obj_meta->class_id == PGIE_CLASS_ID_PERSON) {
        person_count++;
        num_rects++;
    }
}
display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
NvOSD_TextParams *txt_params  = &display_meta->text_params[0];
display_meta->num_labels = 1;
txt_params->display_text = g_malloc0 (MAX_DISPLAY_LEN);
offset = snprintf(txt_params->display_text, MAX_DISPLAY_LEN, "Person = %d ", person_count);
offset = snprintf(txt_params->display_text + offset , MAX_DISPLAY_LEN, "Vehicle = %d ", vehicle_count);

/* Now set the offsets where the string should appear */
txt_params->x_offset = 10;
txt_params->y_offset = 12;

/* Font , font-color and font-size */
txt_params->font_params.font_name = "Serif";
txt_params->font_params.font_size = 10;
txt_params->font_params.font_color.red = 1.0;
txt_params->font_params.font_color.green = 1.0;
txt_params->font_params.font_color.blue = 1.0;
txt_params->font_params.font_color.alpha = 1.0;

/* Text background color */
txt_params->set_bg_clr = 1;
txt_params->text_bg_clr.red = 0.0;
txt_params->text_bg_clr.green = 0.0;
txt_params->text_bg_clr.blue = 0.0;
txt_params->text_bg_clr.alpha = 1.0;

nvds_add_display_meta_to_frame(frame_meta, display_meta);
}

g_print ("Frame Number = %d Number of objects = %d "
    "Vehicle Count = %d Person Count = %d\n",
    frame_number, num_rects, vehicle_count, person_count);
frame_number++;
return GST_PAD_PROBE_OK;
}

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  switch (GST_MESSAGE_TYPE (msg)) {
case GST_MESSAGE_EOS:
  g_print ("End of stream\n");
  g_main_loop_quit (loop);
  break;
case GST_MESSAGE_ERROR:{
  gchar *debug;
  GError *error;
  gst_message_parse_error (msg, &error, &debug);
  g_printerr ("ERROR from element %s: %s\n",
  GST_OBJECT_NAME (msg->src), error->message);
  if (debug)
g_printerr ("Error details: %s\n", debug);
  g_free (debug);
  g_error_free (error);
  g_main_loop_quit (loop);
  break;
}
default:
  break;
  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
 
  GstElement *source = NULL, *h264parser = NULL,
  *decoder = NULL, *streammux = NULL, *sink = NULL, *pgie = NULL, *nvvidconv = NULL,
  *nvosd = NULL; 
#ifdef PLATFORM_TEGRA
  GstElement *transform = NULL;
#endif
  GstBus *bus = NULL;
  guint bus_watch_id;
  GstPad *osd_sink_pad = NULL;
  GstPad *osd_src_pad = NULL;

  /* Check input arguments */
  if (argc != 2) {
g_printerr ("Usage: %s <H264 filename>\n", argv[0]);
return -1;
  }

  /* Standard GStreamer initialization */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create gstreamer elements */
  /* Create Pipeline element that will form a connection of other elements */
  pipeline = gst_pipeline_new ("dstest1-pipeline");

  /* Source element for reading from the file */
  source = gst_element_factory_make ("filesrc", "file-source");

  /* Since the data format in the input file is elementary h264 stream,
   * we need a h264parser */
  h264parser = gst_element_factory_make ("h264parse", "h264-parser");

  /* Use nvdec_h264 for hardware accelerated decode on GPU */
  decoder = gst_element_factory_make ("nvv4l2decoder", "nvv4l2-decoder");

  /* Create nvstreammux instance to form batches from one or more sources. */
  streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");

  if (!pipeline || !streammux) {
g_printerr ("One element could not be created. Exiting.\n");
return -1;
  }

  /* Use nvinfer to run inferencing on decoder's output,
   * behaviour of inferencing is set through config file */
  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

  /* Use convertor to convert from NV12 to RGBA as required by nvosd */
  nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");

  /* Create OSD to draw on the converted RGBA buffer */
  nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");

  /* Finally render the osd output */
#ifdef PLATFORM_TEGRA
  transform = gst_element_factory_make ("nvegltransform", "nvegl-transform");
#endif
 sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");
 //sink = gst_element_factory_make ("filesink", "filesink");
  if (!source || !h264parser || !decoder || !pgie
  || !nvvidconv || !nvosd || !sink) {
g_printerr ("One element could not be created. Exiting.\n");
return -1;
  }

#ifdef PLATFORM_TEGRA
  if(!transform) {
g_printerr ("One tegra element could not be created. Exiting.\n");
return -1;
  }
#endif

  /* we set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", argv[1], NULL);

  g_object_set (G_OBJECT (streammux), "batch-size", 1, NULL);

  g_object_set (G_OBJECT (streammux), "width", MUXER_OUTPUT_WIDTH, "height",
  MUXER_OUTPUT_HEIGHT,
  "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);
  //g_object_set (G_OBJECT (sink), "location", "test1.mp4", NULL);
  /* Set all the necessary properties of the nvinfer element,
   * the necessary ones are : */
  g_object_set (G_OBJECT (pgie),
  "config-file-path", "dstest1_pgie_config.txt", NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* Set up the pipeline */
  /* we add all elements into the pipeline */
#ifdef PLATFORM_TEGRA
  gst_bin_add_many (GST_BIN (pipeline),
  source, h264parser, decoder, streammux, pgie,
  nvvidconv, nvosd, transform, sink, NULL);
#else
  gst_bin_add_many (GST_BIN (pipeline),
  source, h264parser, decoder, streammux, pgie,
  nvvidconv, nvosd, sink, NULL);
#endif

  GstPad *sinkpad, *srcpad;
  gchar pad_name_sink[16] = "sink_0";
  gchar pad_name_src[16] = "src";

  sinkpad = gst_element_get_request_pad (streammux, pad_name_sink);
  if (!sinkpad) {
g_printerr ("Streammux request sink pad failed. Exiting.\n");
return -1;
  }

  srcpad = gst_element_get_static_pad (decoder, pad_name_src);
  if (!srcpad) {
g_printerr ("Decoder request src pad failed. Exiting.\n");
return -1;
  }

  if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
  g_printerr ("Failed to link decoder to stream muxer. Exiting.\n");
  return -1;
  }

  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  /* we link the elements together */
  /* file-source -> h264-parser -> nvh264-decoder ->
   * nvinfer -> nvvidconv -> nvosd -> video-renderer */

  if (!gst_element_link_many (source, h264parser, decoder, NULL)) {
g_printerr ("Elements could not be linked: 1. Exiting.\n");
return -1;
  }

#ifdef PLATFORM_TEGRA
  if (!gst_element_link_many (streammux, pgie,
  nvvidconv, nvosd, transform, sink, NULL)) {
g_printerr ("Elements could not be linked: 2. Exiting.\n");
return -1;
  }
#else
  if (!gst_element_link_many (streammux, pgie,
  nvvidconv, nvosd, sink, NULL)) {
g_printerr ("Elements could not be linked: 2. Exiting.\n");
return -1;
  }
#endif

  /* Lets add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the osd element, since by that time, the buffer would have
   * had got all the metadata. */
  osd_sink_pad = gst_element_get_static_pad (nvosd, "sink");
  if (!osd_sink_pad)
g_print ("Unable to get sink pad\n");
  else
gst_pad_add_probe (osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
osd_sink_pad_buffer_probe, NULL, NULL);
  
  osd_src_pad = gst_element_get_static_pad(nvosd, "src");
if (!osd_src_pad)
g_print("Unable to get OSD src pad \n");
else
gst_pad_add_probe(osd_src_pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
                                     eos_probe_cb, pipeline, NULL);

  /* Set the pipeline to "playing" state */
  g_print ("Now playing: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait till pipeline encounters an error or EOS */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
  return 0;
}