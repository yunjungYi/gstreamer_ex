#include <gst/gst.h>
#include <sys/stat.h>
#include <glib/gstdio.h>
#include <gst/rtsp-server/rtsp-server.h>

#define MOUNT_PATH  "/babo2/live"
#define SHM_PATH  "/tmp/babo2/live"
#define DIR_PATH "/tmp/babo2"

#define S_IRWXUGO (S_IRWXU|S_IRWXG|S_IRWXO)

/**
 * @file rtspserver.cpp
 * @author erica
 * @brief make your own rtspserver with gstreamer
*/

GMainLoop *loop;

struct SrcObject{
  //GstElement *pipeline, *source, *queue1, *depay, *parse, *decoder, *queue2, *conv, *sclcaps, *sink;
  GstElement *pipeline, *source, *queue1, *depay, *parse, *queue2, *sclcaps1, *sclcaps2, *sink;
  GstBus* bus;
  guint bus_watch_id;  
  bool isempty;
};

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  //GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:{      
      SrcObject* t1 = (SrcObject*) data;
      g_print ("End of stream, state : %d\n",GST_STATE(t1->pipeline));      // 카메라 전원 뽑았을 때, 캐치 가능!
      GstStateChangeReturn ret = gst_element_set_state (t1->pipeline, GST_STATE_PAUSED);
      //GstStateChangeReturn ret = gst_element_set_state (t1->pipeline, GST_STATE_PAUSED);
      gst_element_send_event(t1->pipeline, gst_event_new_eos());
      
      //gst_element_set_state (t1->pipeline, GST_STATE_NULL);
      //g_print ("End of stream, state : %d\n",GST_STATE(t1->pipeline));      // 카메라 전원 뽑았을 때, 캐치 가능!
      //g_main_loop_quit (loop);

      // if (ret == GST_STATE_CHANGE_FAILURE) {
      //   g_printerr ("Unable to set the pipeline to the playing state.\n");
      // } else if (ret == GST_STATE_CHANGE_NO_PREROLL) {
      //   g_printerr ("GST_STATE_CHANGE_NO_PREROLL.\n");
      // }
      break;
    }

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
  g_print ("Dynamic pad created, linking src-sink : element %s\n", gst_element_get_name(sink));

  sinkpad = gst_element_get_static_pad (sink, "sink");
  //sinkpad = gst_element_get_request_pad (sink, "front-queue");

  //gst_pad_link (pad, sinkpad);

  /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sinkpad)) {
    g_print ("We are already linked. Ignoring.\n");
    }
  

  if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK) {
    g_print ("Failed to link sinkpad? to pipeline\n");
  }else{
     g_print ("sinkpad? linked to pipeline\n");
  }

  GST_OBJECT_FLAG_SET (sinkpad, GST_PAD_FLAG_PROXY_CAPS);
  GST_OBJECT_FLAG_SET (sinkpad, GST_PAD_FLAG_PROXY_ALLOCATION);


  gst_object_unref (sinkpad);
}



bool CreateSrcObject(SrcObject* s)
{  
  /* Create gstreamer elements */
  s->pipeline = gst_pipeline_new ("player");
  s->source   = gst_element_factory_make ("rtspsrc",       "rtspsource");  
  s->queue1   = gst_element_factory_make ("queue",         "front-queue"); 
  s->depay    = gst_element_factory_make ("rtph264depay",  "depay");
  s->parse    = gst_element_factory_make ("h264parse",     "parser");
  s->queue2   = gst_element_factory_make ("queue",         "back-queue"); 
  s->sclcaps1  = gst_element_factory_make ("capsfilter",    "scale-filterf");
  s->sclcaps2  = gst_element_factory_make ("capsfilter",    "scale-filterb");
  s->sink     = gst_element_factory_make ("shmsink",      "sharedsink");
  /*
  filter   = gst_element_factory_make("capsfilter", "filter");
  g_assert (filter != NULL); // should always exist 
  decoder  = gst_element_factory_make ("omxh264dec",    "decoder");
  queue2   = gst_element_factory_make ("queue",         "back-queue");
  conv     = gst_element_factory_make ("nvvidconv",     "scaler");
  //s->sink     = gst_element_factory_make ("fakesink",   "fakesink");
  */
  
  //gchar* url = "rtsp://admin:gigaeyes%21%40@192.168.0.151:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif";
  //gchar* url = "rtsp://211.54.3.138:1935/gigaeyeslive/cam0000000282.stream";
  //gchar* url = "rtsp://admin:gigaeyes%21%40@192.168.0.84:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif";
  //gchar* url ="rtsp://admin:rlrksurv%40%21@192.168.0.202:/onvif/profile1/media.smp";
  //gchar* url ="rtsp://admin:admin@192.168.0.194:554/stream1";
  //gchar* url="rtsp://admin:rlrksurv%40%21@192.168.22.3:554/onvif/profile2/media.smp";
  // gchar* url="rtsp://admin:rlrksurv%40%21@192.168.22.3:554/onvif/profile2/media.smp";
   gchar* url="rtsp://192.168.33.5:1936/gigaeyeslive/0000010482_200000104821032.stream";
 
  g_printerr("url : %s\n", url);

  if (!s->pipeline || !s->source || !s->queue1 || !s->depay  ||  !s->parse || !s->queue2 || !s->sclcaps2  || !s->sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    goto Create_ERROR;
  }

  g_object_set (G_OBJECT (s->source), "location", url, NULL); 
  g_object_set (G_OBJECT (s->source), "latency", 200, NULL);
  g_object_set (G_OBJECT (s->source), "do-rtcp", FALSE, NULL);
  g_object_set (G_OBJECT (s->source), "do-rtsp-keep-alive", TRUE, NULL);
  g_object_set (G_OBJECT (s->source), "drop-on-latency", TRUE, NULL);
  g_object_set (G_OBJECT (s->source), "max-rtcp-rtp-time-diff", 1000, NULL);  //의심됨...
  g_object_set (G_OBJECT (s->source), "ntp-time-source", 0, NULL);
  g_object_set (G_OBJECT (s->source), "tcp-timeout", 5000000, NULL);
  g_object_set (G_OBJECT (s->source), "teardown-timeout", 100000000, NULL);
  g_object_set (G_OBJECT (s->source), "buffer-mode", 0, NULL);


  //g_object_set (G_OBJECT (s->queue1), "flush-on-eos", TRUE, NULL);  
  //g_object_set (G_OBJECT (s->queue1), "silent", TRUE, NULL);  
  //g_object_set (G_OBJECT (s->queue1), "leaky", 2, NULL);  
  //g_object_set (G_OBJECT (s->queue1), "max-size-bytes", 1048576, NULL);  
  //g_object_set (G_OBJECT (s->queue1), "max-size-buffers", 0, NULL); 
  //g_object_set (G_OBJECT (s->queue1), "max-size-time", 0, NULL); 


  g_object_set (G_OBJECT (s->queue2), "flush-on-eos", TRUE, NULL);  
  //g_object_set (G_OBJECT (s->queue2), "silent", TRUE, NULL);  
  g_object_set (G_OBJECT (s->queue2), "leaky", 2, NULL);  
  g_object_set (G_OBJECT (s->queue2), "max-size-bytes", 5242880, NULL);  
  //g_object_set (G_OBJECT (s->queue2), "max-size-buffers", 0, NULL); 
  //g_object_set (G_OBJECT (s->queue2), "max-size-time", 0, NULL);
  g_object_set (G_OBJECT (s->queue2), "silent", TRUE, NULL);


  g_object_set (G_OBJECT (s->parse), "config-interval", -1, NULL);  
  g_object_set (G_OBJECT (s->parse), "disable-passthrough", TRUE, NULL);

  g_object_set (G_OBJECT (s->sink), "socket-path", SHM_PATH, NULL);  
  g_object_set (G_OBJECT (s->sink), "wait-for-connection", FALSE, NULL);
  g_object_set (G_OBJECT (s->sink), "shm-size", 1048576, NULL);
  g_object_set (G_OBJECT (s->sink), "enable-last-sample", FALSE, NULL);
  g_object_set (G_OBJECT (s->sink), "sync", FALSE, NULL);

  GstCaps *caps1 = gst_caps_new_simple(
                    "video/x-h264",
                    "stream-format", G_TYPE_STRING, "byte-stream",
                    "alignment", G_TYPE_STRING, "nal",
                    NULL);
  GstCaps *caps2 = gst_caps_new_simple(
                    "video/x-h264",
                    "stream-format", G_TYPE_STRING, "byte-stream",
                    "alignment", G_TYPE_STRING, "au",
                    NULL);
  //의심됨 nal? au?
  g_object_set (G_OBJECT (s->sclcaps1),
                  "caps", caps1,
                  NULL);
  g_object_set (G_OBJECT (s->sclcaps2),
                  "caps", caps2,
                  NULL);

  //gst_caps_unref(caps);

  s->bus = gst_pipeline_get_bus (GST_PIPELINE (s->pipeline));
  s->bus_watch_id = gst_bus_add_watch (s->bus, bus_call, s);
  //gst_object_unref (s->bus);

  //gst_bin_add_many (GST_BIN (s->pipeline),
  //                  s->source, s->queue1,  s->depay, s->parse, s->queue2, s->sclcaps, s->sink, NULL);
  gst_bin_add_many (GST_BIN (s->pipeline),
                    s->source,s->queue1, s->depay,  s->sclcaps1, s->parse, s->sclcaps2, s->sink, NULL);                    

  //gst_element_link_many (s->queue1, s->depay, s->parse, s->queue2, s->sclcaps, s->sink, NULL);
  gst_element_link_many (s->queue1, s->depay,s->sclcaps1, s->parse, s->sclcaps2, s->sink, NULL);
  g_signal_connect (s->source, "pad-added", G_CALLBACK (on_pad_added), s->queue1);    // pad-added 언제 발생 ? 
  

  return true;

  Create_ERROR:
  return false;
}

bool DeleteObject(SrcObject* s){
  gst_object_unref (GST_OBJECT (s->pipeline));
  g_source_remove (s->bus_watch_id);
  return true;
}


/*********************************************************************/
/***********************TIMEOUT FUNC LIST ****************************/
/*********************************************************************/ 

// CleanExpiredSession
// client_filter : shutdown all client 
// shutdown_timeout : remove mount path 
// client_connected 

static gint
MakeSrcObject (SrcObject *t1)
{
  // if(GST_STATE(t1->pipeline) == GST_STATE_NULL){
  //   g_print ("Now playing\n");
  //   CreateSrcObject(t1);
  //   gst_element_set_state (t1->pipeline, GST_STATE_PLAYING);
  // }
  // else{
  //   g_print ("Now stopping\n");
  //   gst_element_set_state (t1->pipeline, GST_STATE_NULL);
  //   DeleteObject(t1);
  // }

  if(t1->isempty){
    g_print ("Now Create & Play\n");
    CreateSrcObject(t1);
    gst_element_set_state (t1->pipeline, GST_STATE_PLAYING);
    t1->isempty=FALSE;
  }
  else{
    //g_print ("PLAYING.....\n");
    g_print ("STATE :%d\n", GST_STATE(t1->pipeline));
    //if(GST_STATE(t1->pipeline)!=GST_STATE_PLAYING){
    //  gst_element_set_state (t1->pipeline, GST_STATE_PLAYING);
    //  g_print ("TRYED TO CHANGE STATE :%d\n", GST_STATE(t1->pipeline));      
    //}      
    
    //gst_element_set_state (t1->pipeline, GST_STATE_NULL);
    //DeleteObject(t1);
    //t1->isempty=TRUE;
  }
  //g_print ("Now playing\n");
  return TRUE;
}

/* this timeout is periodically run to clean up the expired sessions from the
 * pool. This needs to be run explicitly currently but might be done
 * automatically as part of the mainloop. */

static GstRTSPFilterResult
client_filter (GstRTSPServer * server, GstRTSPClient * client,
    gpointer user_data)
{
  /* Simple filter that shuts down all clients. */
  return GST_RTSP_FILTER_REMOVE;
}

static gboolean
CleanExpiredSession (GstRTSPServer * server)
{   
  GstRTSPSessionPool *pool;
  pool = gst_rtsp_server_get_session_pool (server); 
  gst_rtsp_session_pool_cleanup (pool);
  g_print("Check expireds session & cleanup : %d\n",gst_rtsp_session_pool_get_n_sessions(pool));  
  g_object_unref (pool);

  return TRUE;
}

void
new_session_cb (GObject * client, GstRTSPSession * session, gpointer user_data)
{
  g_print ("%p: new session %p\n", client, session);
  gst_rtsp_session_set_timeout(session, 3);
}

/* Timeout that runs 10 seconds after the first client connects and triggers
 * the shutdown of the server */
static gboolean
CleanClientinServer (GstRTSPServer * server)
{  
  /* Filter existing clients and remove them */
  g_print ("Force to Disconnect existing clients\n");
  GList * ClientList = gst_rtsp_server_client_filter (server, client_filter, NULL);
  g_print("Now, num of clients in server %d\n", g_list_length(ClientList));
  g_free (ClientList);

  return FALSE;
}

static void GstRTSPClient_closed_cb(GstRTSPClient * self){
  g_print("closed client\n");
}
static void GstRTSPClient_teardown_request_cb(GstRTSPClient * self){
  g_print("teardown requested\n");
}

static void
client_connected (GstRTSPServer * server, GstRTSPClient * client)
{
  //g_print("Client_Connected\n");
  g_signal_connect (G_OBJECT (client), "new-session",
       G_CALLBACK (new_session_cb), NULL);
  g_signal_connect (G_OBJECT (client), "closed",
       G_CALLBACK (GstRTSPClient_closed_cb), NULL);
  g_signal_connect (G_OBJECT (client), "teardown-request",
       G_CALLBACK (GstRTSPClient_teardown_request_cb), NULL);
  //  g_signal_connect (G_OBJECT (client), "pre-teardown-request",
  //     G_CALLBACK (GstRTSPClient_teardown_request_cb), NULL);

  //if (exit_timeout_id == 0) {
  //  g_print ("First Client connected. Disconnecting everyone in 10 seconds\n");
  //  exit_timeout_id =
  //      g_timeout_add_seconds (10, (GSourceFunc) shutdown_timeout, server);
  //}
}

static void
media_configure (GstRTSPMediaFactory * factory, GstRTSPMedia * media)
{
  g_print("media_configure\n");
  gst_rtsp_media_set_reusable (media, TRUE);
  gst_rtsp_media_set_eos_shutdown (media, TRUE);
  gst_rtsp_media_set_stop_on_disconnect (media,TRUE);
}


int main (int argc, char *argv[])
{
  //GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;

  guint id;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  /* create a server instance */
  server = gst_rtsp_server_new ();
  g_object_set (server, "service", "8551", NULL); // monit 8554, unitdev 8553, test 8552

  gst_rtsp_server_set_address (server,"192.168.33.53");


  /* get the mount points for this server, every server has a default object
   * that be used to map uri mount points to media factories */
  mounts = gst_rtsp_server_get_mount_points (server);

  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines. 
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  factory = gst_rtsp_media_factory_new ();

  if(g_remove (SHM_PATH)==0)
    g_print("DELETE_SHM_PATH\n");
  else
    g_print("DELETE_FAIL_SHM_PATH\n");

  if(g_mkdir(DIR_PATH, S_IRWXUGO))
  {
    g_print("CREATE_DIR_PATH_SUCCESS\n");
  }
  else{
    g_print("DIR_EXSIT\n");
  }


  /* TEST SRC  */
  //gst_rtsp_media_factory_set_launch (factory,
  //    "( videotestsrc is-live=1 ! x264enc ! rtph264pay name=pay0 pt=96 )");
  /* SHM SRC  */

  gchar* media_pipeline = g_strdup_printf("shmsrc socket-path=%s do-timestamp=TRUE is-live=TRUE"
             " ! video/x-h264, width=(int)1, height=(int)1, framerate=(fraction)0/1, stream-format=(string)byte-stream, alignment=au"
             " ! queue flush-on-eos=TRUE leaky=2 " 
             " ! rtph264pay config-interval=-1 name=pay0 pt=97", SHM_PATH);
             //           " ! queue flush-on-eos=TRUE max-size-bytes=65536 leaky=2 silent=TRUE "

  //gst_rtsp_media_factory_set_shared (factory, TRUE); // Default: TRUE
  gst_rtsp_media_factory_set_suspend_mode (factory, GST_RTSP_SUSPEND_MODE_RESET); // Default: GST_RTSP_SUSPEND_MODE_NONE
  gst_rtsp_media_factory_set_eos_shutdown(factory, TRUE); // Default: FALSE
  gst_rtsp_media_factory_set_latency (factory, 200); // Default: 2000
  gst_rtsp_media_factory_set_buffer_size (factory, 65536); // Default: 524288 > 512 times
  gst_rtsp_media_factory_set_eos_shutdown (factory, TRUE); // Default: TRUE
  // gst_rtsp_media_factory_set_stop_on_disconnect (rtsp_factory, FALSE); // Default: TRUE
  // gst_rtsp_media_factory_set_retransmission_time(rtsp_factory, 10); // Default: 0
  // gst_rtsp_media_factory_set_do_retransmission(rtsp_factory, TRUE); // Default: FALSE 

  
  gst_rtsp_media_factory_set_launch (factory, media_pipeline);  
  g_signal_connect (factory, "media-configure", (GCallback) media_configure, NULL);


  /* attach the test factory to the /test url */
  gst_rtsp_mount_points_add_factory (mounts, MOUNT_PATH, factory);
  /* Configure if media created from this factory should be stopped
    * when a client disconnects without sending TEARDOWN. */   
  gst_rtsp_media_factory_set_stop_on_disconnect(factory, TRUE);
  //g_print("suspend_mode : %d\n", gst_rtsp_media_factory_get_suspend_mode (factory));
  /* don't need the ref to the mapper anymore */  
  g_object_unref (mounts);

  /* attach the server to the default maincontext */
  if ((id = gst_rtsp_server_attach (server, NULL)) == 0)
    goto ERROR;

  int port = gst_rtsp_server_get_bound_port (server);
  gchar* ip = gst_rtsp_server_get_address (server);
  g_print ("Server(%s:%d) CREATED!\n", ip, port);
  g_free (ip);

  /* start serving */
  g_print ("stream ready at %d:%s\n",port,MOUNT_PATH);


  /* Connect cam */
  struct SrcObject t1;
  t1.isempty = TRUE;  
  //MakeSrcObject(&t1);


  /* Connect signal to Server */
  g_signal_connect (server, "client-connected", (GCallback) client_connected, NULL);
  g_timeout_add_seconds (5, (GSourceFunc) CleanExpiredSession, server);
  g_timeout_add_seconds (5, (GSourceFunc) MakeSrcObject, &t1);


  g_main_loop_run (loop);

  //EXIT:
  //g_main_loop_unref (mainloop);
  //gst_deinit();
  
  //return false;

  /* CLEAN UP */
  g_source_remove (id);
  g_object_unref (server);
  g_main_loop_unref (loop);

  /* RM DIR */ 
  


  ERROR:
  g_print ("ERROR OCCURED\n");
  //goto EXIT;

  return 0;
}
