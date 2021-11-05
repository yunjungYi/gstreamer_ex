#include <gst/gst.h>
#include <glib.h>

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}


static void
on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
  GstPad *sinkpad;
  GstElement *sink = (GstElement *) data;

  /* We can now link this pad */
  g_print ("Dynamic pad created, linking src-sink\n");

  sinkpad = gst_element_get_static_pad (sink, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

static gint
on_timeout_switch ( GstElement *pipeline )
{
  if(GST_STATE(pipeline) == GST_STATE_NULL){
    g_print ("Now playing\n");
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
  }
  else{
    g_print ("Now stopping\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);
  }
  //g_print ("Now playing\n");
  return TRUE;
}

int
main (int   argc,
      char *argv[])
{
  int ret = 0;
  GMainLoop *loop;

  GstElement *pipeline, *source, *queue1, *depay, *parse, *decoder, *queue2, *conv, *sclcaps, *sink;
  GstElement *filter;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);


  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <URI>\n", argv[0]);
    goto ERROR;
  }


  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("player");
  source   = gst_element_factory_make ("rtspsrc",       "rtspsourcee");  
  //queue1   = gst_element_factory_make ("queue",         "front-queue"); // => src에서 받아서 depay후 queue에서 queing 중이라 1분지나면 끊어버림 
  depay    = gst_element_factory_make ("rtph264depay",  "depay");
  parse    = gst_element_factory_make ("h264parse",     "parser");
  //filter   = gst_element_factory_make("capsfilter", "filter");
  //g_assert (filter != NULL); /* should always exist */
  //decoder  = gst_element_factory_make ("omxh264dec",    "decoder");
  //queue2   = gst_element_factory_make ("queue",         "back-queue");
  //conv     = gst_element_factory_make ("nvvidconv",     "scaler");
  //sclcaps  = gst_element_factory_make ("capsfilter",    "scale-filter");
  sink     = gst_element_factory_make ("fakesink",   "fakesink");

  // if (!pipeline || !source || !queue1 || !depay || !parse || !decoder || !queue2 || !conv || !sclcaps || !sink) {
  //   g_printerr ("One element could not be created. Exiting.\n");
  //   goto ERROR;
  // }

  if (!pipeline || !depay  || !parse || !source || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    goto ERROR;
  }



  /* Set up the pipeline */
  /* TRIAL CODE */
  /* we set the input to the source element */
  /* setting rtspsrc property */
  g_object_set (G_OBJECT (source), "location", argv[1], NULL);
  //g_object_set (G_OBJECT (source), "protocols", 4, NULL);
  g_object_set (G_OBJECT (source), "latency", 50, NULL);
  g_object_set (G_OBJECT (source), "do-rtcp", TRUE, NULL);
  g_object_set (G_OBJECT (source), "do-rtsp-keep-alive", TRUE, NULL);
  g_object_set (G_OBJECT (source), "drop-on-latency", TRUE, NULL);

  // g_object_set (G_OBJECT (queue1), "flush-on-eos", TRUE, NULL);
  // g_object_set (G_OBJECT (queue1), "silent", TRUE, NULL);
  // g_object_set (G_OBJECT (queue1), "leaky", 2, NULL);
  // g_object_set (G_OBJECT (queue1), "max-size-bytes", 524288, NULL);
  // g_object_set (G_OBJECT (queue1), "max-size-buffers", 0, NULL);
  // g_object_set (G_OBJECT (queue1), "max-size-time", 0, NULL);

  //g_object_set (G_OBJECT (parse), "config-interval", -1, NULL);
  //g_object_set (G_OBJECT (parse), "disable-passthrough", TRUE, NULL);

  

  //g_object_set (G_OBJECT (source), "tcp-timeout", 10000000, NULL);
  

  /* Fix the output stream resolution */
  // GstCaps *caps = gst_caps_new_simple("video/x-raw",
  //                             "stream-format", G_TYPE_STRING, "byte-stream",
  //                             "alignment", G_TYPE_STRING, "au",
  //                             NULL);
  // g_object_set (G_OBJECT (sclcaps),
  //                "caps", caps,
  //                NULL);
  //  gst_caps_unref(caps);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  //g_signal_connect (bus, "message", G_CALLBACK (cb_stream_message), self);
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* we add all elements into the pipeline */
  //gst_bin_add_many (GST_BIN (pipeline),
  //                  source, queue1, depay, parse, decoder, queue2, conv, sclcaps, sink, NULL);

  gst_bin_add_many (GST_BIN (pipeline),
                    source, depay, parse, sink, NULL);

  //GstCaps* filtercaps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "byte-stream", "alignment", G_TYPE_STRING, "nal", NULL);
  //gst_element_link_filtered(source,depay,filtercaps);
  //g_object_set (G_OBJECT (filter), "caps", filtercaps, NULL);
  //gst_caps_unref (filtercaps);

  /* we link the elements together */
  //gst_element_link_many (queue1, depay, parse, decoder, queue2, conv, sclcaps, sink, NULL);
  gst_element_link_many (depay, parse, sink, NULL);
  g_signal_connect (source, "pad-added", G_CALLBACK (on_pad_added), depay);

  /* Set the pipeline to "playing" state*/
  g_print ("Now playing: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Start/Stop streaming every 10 sec. */
  g_timeout_add_seconds(10, (GSourceFunc)on_timeout_switch, pipeline);
  //g_signal_connect (bus, "message", G_CALLBACK (cb_record_message), self);

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);

EXIT:
  g_main_loop_unref (loop);
  gst_deinit();

  return ret;

ERROR:
  ret = -1;
  goto EXIT;
}