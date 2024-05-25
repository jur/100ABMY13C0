#!/bin/sh
# 6in4.sh - IPv6-in-IPv4 tunnel backend
# Copyright (c) 2010-2014 OpenWrt.org

[ -n "$INCLUDE_ONLY" ] || {
	[ -f /lib/functions.sh ] && {
		. /lib/functions.sh
	} || {
		. /sbin/functions.sh
	}
}

find_6in4_wanif() {
	sleep 2
	local if=$(ip -4 r l e 0.0.0.0/0); if="${if#default* dev }"; if="${if%% *}"
	[ -n "$if" ] && grep -qs "^ *$if:" /proc/net/dev && echo "$if"
}

find_6in4_wanip() {
	local ip=$(ip -4 a s dev "$1"); ip="${ip#*inet }"
	echo "${ip%%[^0-9.]*}"
}

setup_interface_6in4() {
	local cfg="$1"
	local iface="$2"
	local link="6in4-$cfg"

	local ck6in4Enable=$(uci get network.general."$cfg"_enable)
	[ "$ck6in4Enable" == "0" ] && return

	local local4=$(uci_get network "$cfg" ipaddr)
	local remote4=$(uci_get network "$cfg" peeraddr)
	local local6=$(uci_get network "$cfg" ip6addr)
	local local6prefix=$(uci_get network "$cfg" ip6prefix)
	local mtu=$(uci_get network "$cfg" mtu)
	local ttl=$(uci_get network "$cfg" ttl)
	local metric=$(uci_get network "$cfg" metric)

	local defaultroute
	config_get_bool defaultroute "$cfg" defaultroute 1

	# If local4 is unset, guess local IPv4 address from the
	# interface used by the default route.
	[ -z "$local4" ] && {
		for i in $(seq 1 5);
		do
			sleep 1
			local wanif=$(find_6in4_wanif)
			#echo "== i=$i wanif=$wanif ===" #> /dev/console
			[ -n "$wanif" ] && {
				local4=$(find_6in4_wanip "$wanif")
				#echo "== local4_2=$local4 ===" #> /dev/console
				uci_set_state network "$cfg" wan_device "$wanif"
				break
			}
		done
	}

	#echo "=== remote4=$remote4 local6=$local6 local6prefix=$local6prefix mtu=$mtu ttl=$ttl metric=$metric ===" #> /dev/console

	[ -n "$local4" ] && {
		# creating the tunnel below will trigger a net subsystem event
		# prevent it from touching or iface by disabling .auto here
		uci_set_state network "$cfg" ifname $link
		uci_set_state network "$cfg" auto 0

		ip tunnel add $link mode sit remote $remote4 local $local4 ttl ${ttl:-64}
		ip link set $link up

		ip link set mtu ${mtu:-1280} dev $link
		ip addr add $local6 dev $link
		
		ip -6 route del default
		ip route add ::/0 dev $link
		#echo "== ip route add ::/0 dev $link ==" #> /dev/console

		uci_set_state network "$cfg" ipaddr $local4
		uci_set_state network "$cfg" ip6addr $local6

		[ "$defaultroute" = 1 ] && {
			local inet="::/0"
			local kern="$(uname -r)"
			[ "${kern#2.4}" != "$kern" ] && inet="2000::/3"

			ip -6 route del default
			ip -6 route add $inet ${metric:+metric $metric} dev $link
			uci_set_state network "$cfg" defaultroute 1
		}
	} || {
		echo "Cannot determine local IPv4 address for 6in4 tunnel $cfg - skipping" #> /dev/console
	}

	[ -n "$tunnelid" -a -n "$username" -a \( -n "$password" -o -n "$updatekey" \) ] && {
		[ -n "$updatekey" ] && password="$updatekey"

		local url="http://ipv4.tunnelbroker.net/nic/update?username=$username&password=$password&hostname=$tunnelid"
		local try=0
		local max=3

		while [ $((++try)) -le $max ]; do
			( exec wget -qO/dev/null "$url" 2>/dev/null ) &
			local pid=$!
			( sleep 5; kill $pid 2>/dev/null ) &
			wait $pid && break
		done
	}

	local ip6addr=$local6
	[ -n "$ip6addr" ] && {
		local local6="${ip6addr%%/*}"
		local mask6="${ip6addr##*/}"
		[[ "$local6" = "$mask6" ]] && mask6=
		#echo "=== local6=$local6 mask6=$mask6 ===" #> /dev/console
	}

	##Add for assign LAN ipv6 address
	local ipv6lan=
	local ip6prefix=$local6prefix
	local ipv6lanold=$(uci get network."$cfg".zyipv6lan)
	local prefix_local6="${ip6prefix%%/*}"
	local prefix_mask6="${ip6prefix##*/}"
	#echo "=== prefix_local6=$prefix_local6 prefix_mask6=$prefix_mask6 ===" #> /dev/console

	local product_name=$(uci get system.main.product_name)
	local config_section=$(echo "$cfg"| sed s/6in4//g)
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
	
	##add 6in4 CPE lan-ipv6 
	ipv6lan="$prefix_local6"1
	
	[ -n "$ipv6lanold" -a "$ipv6lanold" != "$ipv6lan" ] && ifconfig $lanIface del "$ipv6lanold"/64
	ifconfig $lanIface add "$ipv6lan"/64

	uci set network."$cfg".zyipv6lan=$ipv6lan
	uci commit network

	echo 1 > /tmp/radvd_6in4
	/etc/init.d/radvd restart
	##END	
}

stop_interface_6in4() {
	#echo "== 6in4_stop 1 ===" #> /dev/console
	local cfg="$1"
	local link="6in4-$cfg"

	local local6=$(uci_get_state network "$cfg" ip6addr)
	local defaultroute=$(uci_get_state network "$cfg" defaultroute)

	grep -qs "^ *$link:" /proc/net/dev && {
		#echo "== 6in4_stop 2 ===" #> /dev/console

		[ "$defaultroute" = "1" ] && {
			local inet="::/0"
			local kern="$(uname -r)"
			[ "${kern#2.4}" != "$kern" ] && inet="2000::/3"

			ip -6 route del $inet dev $link
		}

		ip addr del $local6 dev $link
		ip link set $link down
		ip tunnel del $link
	}
}

[ -n "$INCLUDE_ONLY" ] || {
	[ -f /lib/netifd/proto/6in4 ] || {
		stop_interface_6in4 wan6in4
	}

	setup_interface_6in4 wan6in4
}
