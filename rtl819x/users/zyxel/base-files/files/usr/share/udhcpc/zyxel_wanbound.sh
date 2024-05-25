#!/bin/sh

GETMIB="flash get"
eval `$GETMIB OP_MODE`

[ "$OP_MODE" == "0" ] && {
  [ -f /config/uci/network ] && {
	if [ $1 == "bound" ]; then
		echo "$interface $ip $router $dns" > /var/udhcpc_wanScan_info
	fi
  }
} || {
	ech "==== zyxel_wanScanProto.sh are only working on Router Mode. ====" > /dev/console
}

