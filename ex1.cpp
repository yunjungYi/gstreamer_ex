#include <gst/gst.h>
#include <glib.h>

static GMainLoop *loop;
//#define URL "rtsp://admin:gigaeyes%21%40@192.168.0.151:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif"
//#define URL "rtsp://211.54.3.138:1935/gigaeyeslive/cam0000000282.stream"
#define URL "rtsp://admin:4321@192.168.22.3:554/onvif/profile2/media.smp"


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
  GstElement *queue = (GstElement *) data;

  /* We can now link this pad */
  g_print ("Dynamic pad created, linking front-queue\n");

  sinkpad = gst_element_get_static_pad (queue, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

struct StreamingObject{
  GstElement *pipeline, *source, *queue1, *depay, *parse, *sclcaps, *sink;
  GstBus *bus;
  guint bus_watch_id;
  bool isempty;
};

bool DeleteObject(StreamingObject* s){
  g_print ("Deleting pipeline\n");

  gst_object_unref (GST_OBJECT (s->pipeline));
  g_source_remove (s->bus_watch_id);
  //g_object_unref(s);
  return true;
}

bool CreateObject(StreamingObject* s){
  g_print ("START CREATE\n");

   /* Create gstreamer elements */
  s->pipeline = gst_pipeline_new ("player");
  s->source   = gst_element_factory_make ("rtspsrc",       "rtsp-source");
  s->queue1   = gst_element_factory_make ("queue",         "front-queue");
  s->depay    = gst_element_factory_make ("rtph264depay",  "rtp-depay");
  s->parse    = gst_element_factory_make ("h264parse",     "parser");
  s->sclcaps  = gst_element_factory_make ("capsfilter",    "scale-filter");
  s->sink     = gst_element_factory_make ("fakesink",   "fakesink");

  if (!s->pipeline || !s->source || !s->queue1 || !s->depay || !s->parse || !s->sclcaps || !s->sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    goto ERR;
  }

  /* Set up the pipeline */

  /* we set the input to the source element */
  g_object_set (G_OBJECT (s->source), "location", URL, NULL); 
  g_object_set (G_OBJECT (s->source), "latency", 50, NULL);
  g_object_set (G_OBJECT (s->source), "do-rtcp", TRUE, NULL);
  g_object_set (G_OBJECT (s->source), "do-rtsp-keep-alive", TRUE, NULL);
  g_object_set (G_OBJECT (s->source), "drop-on-latency", TRUE, NULL);
  
  

  // /* Fix the output stream resolution */
  // GstCaps *caps = gst_caps_new_simple("video/x-raw",
  //                            "width", G_TYPE_INT, 800,
  //                            "height", G_TYPE_INT, 450,
  //                            NULL);
  // g_object_set (G_OBJECT (s->sclcaps),
  //               "caps", caps,
  //               NULL);
  // gst_caps_unref(caps);

  /* we add a message handler */
  s->bus = gst_pipeline_get_bus (GST_PIPELINE (s->pipeline));
  s->bus_watch_id = gst_bus_add_watch (s->bus, bus_call, loop);
  gst_object_unref (s->bus);
  

  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (s->pipeline),
                    s->source, s->queue1, s->depay, s->parse, s->sclcaps, s->sink, NULL);

  /* we link the elements together */
  gst_element_link_many (s->queue1, s->depay, s->parse, s->sclcaps, s->sink, NULL);
  g_signal_connect (s->source, "pad-added", G_CALLBACK (on_pad_added), s->queue1);
 
  return true;

  ERR:
  return false;
}


on_timeout_switch (StreamingObject* s)
{

  g_print ("ON_TIME_SWITCH\n");

  if(s->isempty){
    g_print ("Now Create & Play\n");
    CreateObject(s);
    gst_element_set_state (s->pipeline, GST_STATE_PLAYING);
    s->isempty=FALSE;
  }
  else{
    g_print ("Now stopping\n");
    gst_element_set_state (s->pipeline, GST_STATE_NULL);
    DeleteObject(s);    
    s->isempty=TRUE;
  }

  return TRUE;
}




int
main (int   argc,
      char *argv[])
{
  int ret = 0;

  gst_init (&argc, &argv);

  /* Check input arguments */
  // if (argc != 2) {
  //   g_printerr ("Usage: %s <URI>\n", argv[0]);
  //   goto ERROR;
  // }

  struct StreamingObject t1;
  t1.isempty = TRUE;
  //CreateObject(&t1);
  
  loop = g_main_loop_new (NULL, FALSE);




  /* Start/Stop streaming every 10 sec. */
  g_timeout_add_seconds(5, (GSourceFunc)on_timeout_switch, &t1);

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");

  

EXIT:
  g_main_loop_unref (loop);
  gst_deinit();
  return ret;

ERROR:
  ret = -1;
  goto EXIT;
}