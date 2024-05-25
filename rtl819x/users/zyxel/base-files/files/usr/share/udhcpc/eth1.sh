#!/bin/sh

[ -n "$ip6rd" ] && echo "$ip6rd" > /var/log/udhcpc_ip6rd.log

#That will support ZYXEL solution, so add them
[ -f /config/uci/network ] && {
	if [ "$1" != "leasefail" ]; then
        	exec /usr/share/udhcpc/eth1.ZYXEL_$1
        	exit 0
	fi
}

if [ "$1" == "leasefail" ]; then
	exit 0
fi

exec /usr/share/udhcpc/eth1.$1
