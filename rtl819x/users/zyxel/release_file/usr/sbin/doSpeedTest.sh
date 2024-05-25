#!/bin/sh

#wsr30
#usage: $0 target <serverlist | lanmac> run_secs [device_mac]
#Example:
#  $0 internet serverlist [device_mac]
#  $0 gateway  target_lanmac
target=$1 #internet or gateway or extender
run_secs=$3 #secs
device_mac=$4 #alexa
notfication_enable=$(uci get notification.speedtest.enable)
speedtest_result_file="/tmp/SpeedTestInfo"

if [ -z "$run_secs" ] || { [ "$target" != "internet" ] && [ "$target" != "gateway" ] && [ "$target" != "extender" ]; }; then
  echo "****error args"
  exit
fi

if [ -z "$device_mac" ]; then # general type
  if [ "$target" = "internet" ]; then
    /usr/bin/SpeedTest -t 4 -c "$run_secs" -f "$2"
  elif [ "$target" = "gateway" ]; then
    /usr/bin/pktgen_control -m "$2" -t "$run_secs"
  elif [ "$target" = "extender" ]; then
    /usr/bin/pktgen_control -m "$2" -t "$run_secs" -s
  fi
else #alexa type
  echo -n "0,0,0" > /tmp/SpeedTestInfo # set status=0 means executing.
  servers_status=$(/usr/bin/SpeedTest -C -f "$2")
  if [ "$servers_status" != "OK" ]; then
    /usr/bin/SpeedTest -g -m > "$2"
  fi
  /usr/bin/SpeedTest -t 4 -c "$run_secs" -f "$2"
  if [ "$notfication_enable" = "1" ]; then
    zlog 7 2 '****Notify phone(from alexa)'

    speedtest_result=`cat $speedtest_result_file`
    if [ -n "$speedtest_result" ]; then
      speedtest_dl=`printf $speedtest_result | cut -f 1 -d ','`
      speedtest_ul=`printf $speedtest_result | cut -f 2 -d ','`
      speedtest_status=`printf $speedtest_result | cut -f 3 -d ','`
      zlog 7 2 "SpeedTest result: downloading rate[$speedtest_dl], uploading rate[$speedtest_ul], status[$speedtest_status]."
      #Since WSR30 does not support push-notification-cli command, using the old fashion way to push notification.
      #push-notification-cli -g "/tmp/.cloudagent/push_notification/app_group_id_for_multy" -s "$speedtest_ul" "$speedtest_dl" "Mbps"
      # Old push notification method
      /usr/bin/SpeedTest -N "$device_mac"
    else
      zlog 3 2 "SpeedTest NotifyPhone, can't open $speedtest_result_file. Or it is an empty file."
    fi
  fi
fi
