#!/bin/sh
#TIME= `uptime | cut -d'u' -f1`

TIME=`date | awk '{print $4}'`

PID=`pidof streamer`
MEM=`ps -eo pid,rss,vsize | grep "41206"`
P="$TIME $MEM"
echo $P >> "/home/yjlee/exmaple/Memstat/Memstat.txt"

