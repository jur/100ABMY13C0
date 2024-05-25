#!/bin/sh

lan_interface=br0
# usage target extender_first_mac run_or_wait_sec path_root filename [extender_lan_mac]
#   target             : 'internet' or 'gateway'
#   extender_first_mac : extender first mac
#   run_or_wait_sec    : exec speedtest/pktgen x secs or pktgen after wait x secs.
#   path_root          : path of server list
#   filename           : server list file name
#   extender_lan_mac   : for pktgen_control"
target=$1
extender_first_mac=$2
run_or_wait_sec=$3
path_root=$4
filename=$5
extender_lan_mac=$6

if [ "$target" = "internet" ]; then
  zy1905App 22 "$extender_first_mac" 1 1 "$path_root" "$filename" "$run_or_wait_sec" >/dev/console
elif [ "$target" = "gateway" ]; then
  lan_mac_myself=$(ifconfig $lan_interface | grep HWaddr | awk '{printf $NF}')
  zy1905App 22 "$extender_first_mac" 1 2 "$path_root" "$filename" "$run_or_wait_sec" "$lan_mac_myself" >/dev/console
else
  echo "
  usage target extender_first_mac run_or_wait_sec path_root filename [extender_lan_mac]
    target             : 'internet' or 'gateway'
    extender_first_mac : extender first mac
    run_or_wait_sec    : exec speedtest/pktgen x secs or pktgen after wait x secs.
    path_root          : path of server list
    filename           : server list file name
    extender_lan_mac   : for pktgen_control"
  exit
fi

count=1
while true
do
  sleep 1
  info=$(cat /tmp/SpeedTestInfo)
  if [ -z "$info" ]; then
    count=$((count + 1))
  elif [ "$(echo "$info" | awk 'BEGIN{FS=","} {print $3}')" = 0 ]; then
    break
  fi

  if [ $count -ge 20 ]; then
    rm -f /tmp/doSpeedTest1905.unlock
    exit
  fi
done


[ -n "$extender_lan_mac" ] && /usr/sbin/doSpeedTest.sh extender "$extender_lan_mac" "$run_or_wait_sec" &
/usr/sbin/doCheck_timeout.sh $((run_or_wait_sec * 2 + 7))
