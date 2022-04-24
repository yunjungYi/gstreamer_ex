#!/bin/sh
#TIME= `uptime | cut -d'u' -f1`

TIME=`date | awk '{print $4}'`

#PID=`pidof valgrind`
APPNAME="RTSPServer_port_8554_2"
 #echo $APPNAME

PID=`pidof $APPNAME`
#PID=$1
 #echo $PID
NUM=3465
MEM=`ps -eo pid,rss,vsize | grep "$PID"`
#MEM=`ps -eo pid,rss,vsize | grep "$NUM"`
P="$TIME $MEM"
#echo $P

echo $P >> "/home/yjlee/exmaple/Memstat/${APPNAME}.txt"

