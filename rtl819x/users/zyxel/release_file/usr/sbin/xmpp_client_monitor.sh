#!/bin/sh

PKGPATH="/usr"

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PKGPATH/lib:$PKGPATH/lib/purple-2

CLOUDAGENT_LOGGER="$PKGPATH/sbin/cloudagent_logger.sh"
START_CLOUDAGENT_SH="$PKGPATH/sbin/start_cloudagent.sh"


while true; do
	sleep 10
	# check xmpp client process
	start_cloudagent_test=$(pidof start_cloudagent.sh)
	if [ -z "$start_cloudagent_test" ];then
		xmpp_client_test=$(pidof zyxel_xmpp_client)
		if [ -z "$xmpp_client_test" ];then
			$($CLOUDAGENT_LOGGER '[XMPP Client][INFO] monitor: agent is dead, start it now')
			$START_CLOUDAGENT_SH > /dev/null 2>&1 &
		fi
	fi
done
