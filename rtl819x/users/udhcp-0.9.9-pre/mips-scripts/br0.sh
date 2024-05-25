#!/bin/sh

#That will support ZYXEL solution, so add them
[ -f /config/uci/network ] && {
        exec /usr/share/udhcpc/br0.ZYXEL_$1
        exit 0
}

exec /usr/share/udhcpc/br0.$1