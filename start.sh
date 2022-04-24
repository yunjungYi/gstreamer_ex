#!/bin/sh

source ./def.include
APP_NAME=$1
kill -9 `pidof $APP_NAME`
cat /dev/null > console.log
cat /dev/null > debug.log
#ulimit -c unlimited
echo $APP_NAME "START"

set_debug()
{
############### GLib debug ##################
# export G_DEBUG=fatal_error #fatal_debug   #### if you want to system stop when log trigger occurred over G_DEBUG level, set this flag
#   export G_MESSAGES_DEBUG=all
#  export G_DEBUG=fatal_warnings
#############################################
  #CNSLDBG_TEMP=$(date +console.%Y-%m-%d-%H_%M_%S_DEBUG.log)
  CNSLDBG=./debug.log
#  echo "set debug"
############### GST debug ###################
#  export GST_DEBUG_DUMP_DOT_DIR="./logs/graphs"
#  export GST_DEBUG=GST_BUFFER:7,GST_MEMORY:5
#  export GST_DEBUG=GST_MEMORY
#  export GST_TRACERS="leaks;latency"
#export GST_DEBUG="GST_TRACER:7"
#export GST_TRACERS="leaks"

export GST_DEBUG=4
#export GST_DEBUG=rtsp*:4,rtspclient:2 #simple rtspmedia:5 
#export GST_DEBUG=4,rtspclient:2 #full
#export G_DEBUG=gc-friendly
export G_SLICE=always-malloc
#export G_SLICE=debug-blocks
export GST_DEBUG_NO_COLOR=1    #### set no color mode
# export GST_DEBUG_COLOR_MODE=unix #### set color mode to unix
export GST_DEBUG_FILE=$CNSLDBG
#    export GST_TRACE=all
#############################################
}

set_debug3()
{
############### GLib debug ##################
# export G_DEBUG=fatal_error #fatal_debug   #### if you want to system stop when log trigger occurred over G_DEBUG level, set this flag
#   export G_MESSAGES_DEBUG=all
#############################################
  #CNSLDBG_TEMP=$(date +console.%Y-%m-%d-%H_%M_%S_DEBUG.log)
  CNSLDBG=./debug.log
#  echo "set debug"
############### GST debug ###################
  export GST_DEBUG_DUMP_DOT_DIR="./graphs"
#  export GST_DEBUG=GST_BUFFER:7,GST_BUFFER_LIST:7,GST_CALL_TRACE:4,GST_MEMORY:7,GST_TRACER:7
  #export GST_DEBUG=GST_REFCOUNTING:5
    export G_DEBUG=all
export GST_LEAKS_TRACER_STACK_TRACE=1
export GST_TRACERS='leaks'
export GST_DEBUG=4
#export GST_DEBUG=GST_TRACER:7
#export GST_DEBUG=GST_PIPELINE:7,GST_DATAFLOW:7
#export GST_DEBUG='GST_BUFFER:4,GST_BUFFER_LIST:4,GST_MEMORY:4,GST_REFCOUNTING:4'
#    export G_SLICE=always-malloc
	export G_SLICE=debug-blocks
  export GST_DEBUG_NO_COLOR=1    #### set no color mode
# export GST_DEBUG_COLOR_MODE=unix #### set color mode to unix
   export GST_DEBUG_FILE=$CNSLDBG
#    export GST_TRACE=all
#############################################
}

set_debug2()
{
	CNSLDBG=./debug.log
	#export GST_LEAKS_TRACER_STACK_TRACE=1
	export GST_DEBUG="GST_TRACER:7"
#	export GST_TRACERS="log(events,buffers);stats(all)"	
#	export GST_TRACERS="meminfo;dbus"
	export GST_TRACERS="leaks(check-refs=true)"
#	export GST_TRACERS="leaks"
#	export GST_LEAKS_TRACER_SIG=TRUE
#	export GST_TRACER="timer(10ms);rusage;stats"	
	export GST_DEBUG_FILE=$CNSLDBG
}


run_with_val(){
SUPP_DIR=/home/yjlee/Lib/supp
G_SLICE=always-malloc G_DEBUG=gc-friendly \
valgrind --leak-check=full --leak-resolution=high --show-leak-kinds=all --num-callers=20 --trace-children=yes --log-file=memcheck.txt  --suppressions=${SUPP_DIR}/gst.supp --suppressions=${SUPP_DIR}/glib.supp ./${APP_NAME}
}

#set_ulimit(){
#  ulimit -m 24576
#  ulimit -d 30720 
#}

run_with_normal(){
#./$APP_NAME >> console.log 2>&1 &
# example valgrind
# valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes ./shell

G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes ./${APP_NAME}  1>> console.log 2>&1 &
}


run_with_val(){
G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
--leak-check=full --leak-resolution=high --num-callers=20 --trace-children=yes \
--suppressions=/home/gigasurv/lib/supp/gst.supp \
--suppressions=/home/gigasurv/lib/supp/glib.supp \
./${APP_NAME}  1>> console.log 2>&1 &
}

#runtarget=$1 
#set_debug2
#set_ulimit
#if ["$2" == "val" ]; then
#	run_with_val
#else
#	run_with_val
#	./$APP_NAME >> console.log 2>&1 &
#fi
#./streamer # & echo $(pidof streamer)
#./ex1 "rtsp://admin:gigaeyes%21%40@192.168.0.151:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif"
#./ex1

set_debug
run_with_val
#run_with_normal
