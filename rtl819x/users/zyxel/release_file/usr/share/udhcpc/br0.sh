#!/bin/sh

#That will support ZYXEL solution, so add them
[ -f /config/uci/network ] && {
	if [ "$1" != "leasefail" ]; then
		exec /usr/share/udhcpc/br0.ZYXEL_$1
		exit 0
	fi
}

if [ "$1" == "leasefail" ]; then
	exit 0
fi

exec /usr/share/udhcpc/br0.$1
