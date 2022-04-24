#!/bin/sh

source ./def.include
rm ./debug.log
cat /dev/null > mem.txt
ulimit -c unlimited

set_debug()
{
############### GLib debug ##################
# export G_DEBUG=fatal_error #fatal_debug   #### if you want to system stop when log trigger occurred over G_DEBUG level, set this flag
   export G_MESSAGES_DEBUG=all
  export G_DEBUG=fatal_warnings
#############################################
  #CNSLDBG_TEMP=$(date +console.%Y-%m-%d-%H_%M_%S_DEBUG.log)
  CNSLDBG=./debug.log
#  echo "set debug"
############### GST debug ###################
  export GST_DEBUG_DUMP_DOT_DIR="./logs/graphs"
#  export GST_DEBUG=GST_BUFFER:7,GST_MEMORY:5
#  export GST_DEBUG=GST_MEMORY:LOG
#  export GST_TRACERS="leaks;latency"
export GST_TRACERS="leaks"
export GST_DEBUG="GST_TRACER:7"
#    export G_DEBUG=gc-friendly
#    export G_SLICE=always-malloc
    export GST_DEBUG_NO_COLOR=1    #### set no color mode
# export GST_DEBUG_COLOR_MODE=unix #### set color mode to unix
   export GST_DEBUG_FILE=$CNSLDBG
#    export GST_TRACE=all
#############################################
}


run_with_val(){
SUPP_DIR=/home/yjlee/Lib/supp
G_SLICE=always-malloc G_DEBUG=gc-friendly \
valgrind --leak-check=full --leak-resolution=high --show-leak-kinds=all --num-callers=20 --trace-children=yes --log-file=memcheck.txt  --suppressions=${SUPP_DIR}/gst.supp --suppressions=${SUPP_DIR}/glib.supp ./${APP_NAME}
}


#set_debug
#./streamer # & echo $(pidof streamer)
./RTSPServer "rtsp://admin:gigaeyes%21%40@192.168.0.151:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif"
