#!/bin/sh
# 6to4.sh - IPv6-in-IPv4 tunnel backend
# Copyright (c) 2010-2012 OpenWrt.org

[ -n "$INCLUDE_ONLY" ] || {
	[ -f /lib/functions.sh ] && {
		. /lib/functions.sh
	} || {
		. /sbin/functions.sh
	}
}

find_6to4_wanif() {
	sleep 2
	local if=$(ip -4 r l e 0.0.0.0/0); if="${if#default* dev }"; if="${if%% *}"
	[ -n "$if" ] && grep -qs "^ *$if:" /proc/net/dev && echo "$if"
}

find_6to4_wanip() {
	local ip=$(ip -4 a s dev "$1"); ip="${ip#*inet }"
	echo "${ip%%[^0-9.]*}"
}

find_6to4_prefix() {
	local ip4="$1"
	local oIFS="$IFS"; IFS="."; set -- $ip4; IFS="$oIFS"

	printf "2002:%02x%02x:%02x%02x\n" $1 $2 $3 $4
}

test_6to4_rfc1918()
{
	local oIFS="$IFS"; IFS="."; set -- $1; IFS="$oIFS"
	[ $1 -eq  10 ] && return 0
	[ $1 -eq 192 ] && [ $2 -eq 168 ] && return 0
	[ $1 -eq 172 ] && [ $2 -ge  16 ] && [ $2 -le  31 ] && return 0

	# RFC 6598
	[ $1 -eq 100 ] && [ $2 -ge  64 ] && [ $2 -le 127 ] && return 0

	return 1
}

proto_6to4_setup() {
	local cfg="$1"
	local iface="$2"
	local link="6to4-$cfg"

	local local4=$(uci_get network "$cfg" ipaddr)
	local mtu=$(uci_get network "$cfg" mtu)
	local ttl=$(uci_get network "$cfg" ttl)
	local metric=$(uci_get network "$cfg" metric)

	local defaultroute
	config_get_bool defaultroute "$cfg" defaultroute 1

	local relayaddr=$(uci get network.wan6to4.relayaddr)
	[ -z "$relayaddr" ] && relayaddr=192.88.99.1	

	# If local4 is unset, guess local IPv4 address from the
	# interface used by the default route.
	[ -z "$local4" ] && {
		for i in $(seq 1 5);
		do
			sleep 1
			local wanif=$(find_6to4_wanif)
			#echo "== wanif=$wanif ===" #> /dev/console
			[ -n "$wanif" ] && {
				local4=$(find_6to4_wanip "$wanif")
				#echo "== local4=$local4 ===" #> /dev/console
				uci_set_state network "$cfg" wan_device "$wanif"
				break
			}
		done
	}

	test_6to4_rfc1918 "$local4" && {
		proto_notify_error "$cfg" "INVALID_LOCAL_ADDRESS"
		return
	}

	local ck6to4Enable=$(uci get network.general."$cfg"_enable)
	[ "$ck6to4Enable" == "0" ] && return

	[ -n "$local4" ] && {
		# creating the tunnel below will trigger a net subsystem event
		# prevent it from touching or iface by disabling .auto here
		uci_set_state network "$cfg" ifname $link
		uci_set_state network "$cfg" auto 0

		# find our local prefix
		local prefix6=$(find_6to4_prefix "$local4")
		local local6="$prefix6::1/16"

		ip tunnel add $link mode sit remote any local $local4 ttl ${ttl:-64}
		ip link set $link up
		ip link set mtu ${mtu:-1280} dev $link
		ip addr add $local6 dev $link

		uci_set_state network "$cfg" ipaddr $local4
		uci_set_state network "$cfg" ip6addr $local6

		[ "$defaultroute" = 1 ] && {
			logger -t "$link" " * Adding default route"
			ip -6 route add ::/0 dev $link
			# Change default route
			ip -6 route change via ::$relayaddr dev $link
			uci_set_state network "$cfg" defaultroute 1
		}

		echo ::$relayaddr >/tmp/6to4_route
	} || {
		echo "Cannot determine local IPv4 address for 6to4 tunnel $cfg - skipping"
	}

	##Add for assign LAN ipv6 address
	local ipv6lan=
	local ipv6lanold=$(uci get network."$cfg".zyipv6lan)
	local ipv6lanprefix="${prefix6}:"

	local product_name=$(uci get system.main.product_name)
	local config_section=$(echo "$cfg"| sed s/6to4//g)
	local lanIface=
	[ "${cfg:0:3}" == "wan" ] && {
		lanIface=$(uci get network."$config_section".bind_LAN)
		lanIface=$(echo $lanIface | cut -c 4-)	
		[ -z "$lanIface" ] && {
			if [ $product_name == "NBG6817" ]; then
				lanIface=br-lan
			else
				lanIface=br0
			fi
		}
	}
	
	##add 6to4 CPE lan-ipv6 
	local mac=$(ifconfig $lanIface | sed -ne 's/[[:space:]]*$//; s/.*HWaddr //p')
	ipv6lan=$ipv6lanprefix:$(printf %02x $((0x${mac%%:*} ^ 2)))
	mac=${mac#*:}
	ipv6lan=$ipv6lan${mac%:*:*:*}ff:fe
	mac=${mac#*:*:}
	ipv6lan=$ipv6lan${mac%:*}${mac##*:}
	
	[ -n "$ipv6lanold" -a "$ipv6lanold" != "$ipv6lan" ] && ifconfig $lanIface del "$ipv6lanold"/64
	ifconfig $lanIface add "$ipv6lan"/64

	uci set network."$cfg".zyipv6lan=$ipv6lan
	uci commit network

	echo 1 > /tmp/radvd_6to4
	/etc/init.d/radvd restart
	##END
}

stop_interface_6to4() {
	local cfg="$1"
	local link="6to4-$cfg"

	local defaultroute=$(uci_get_state network "$cfg" defaultroute)

	grep -qs "^ *$link:" /proc/net/dev && {
		[ "$defaultroute" = "1" ] && {
			ip -6 route del dev $link
		}

		ip link set $link down
		ip tunnel del $link
	}
}

[ -n "$INCLUDE_ONLY" ] || {
	[ -f /lib/netifd/proto/6to4 ] || {
		stop_interface_6to4 wan6to4
	}
	proto_6to4_setup wan6to4
}
