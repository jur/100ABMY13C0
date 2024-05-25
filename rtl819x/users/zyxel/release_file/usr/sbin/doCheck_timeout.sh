#!/bin/sh

time_out="$1"
[ -z "$time_out" ] && time_out=27

start_time=$(date +%s)
count=1
while true
do
  sleep 3
  stop_time=$(date +%s)
  info=$(cat /tmp/SpeedTestInfo)
  info2=$(cat /tmp/ApplicationData/SpeedTestInfo)
  if [ -z "$info" ]; then
    count=$((count + 1))
  elif [ "$(echo "$info" | awk 'BEGIN{FS=","} {print $3}')" = 1 ]; then
    rm -f /tmp/doSpeedTest1905.unlock
    break
  elif [ $((stop_time - start_time)) -ge $time_out ]; then
    rm -f /tmp/doSpeedTest1905.unlock
    sleep 1
    echo -n "$(echo "$info" | awk 'BEGIN{FS=","} {if ($3 == 0) print $1","$2","1 ; else print $1","$2","$3}')" > /tmp/SpeedTestInfo
    echo -n "$(echo "$info2" | awk 'BEGIN{FS=","} {if ($3 == 0) print $1","$2","1 ; else print $1","$2","$3}')" > /tmp/ApplicationData/SpeedTestInfo
    sync
  fi

  if [ $count -ge 10 ]; then
    rm -f /tmp/doSpeedTest1905.unlock
    sleep 1
    echo -n "0,0,1" > /tmp/SpeedTestInfo
    sync
    break
  fi
done
