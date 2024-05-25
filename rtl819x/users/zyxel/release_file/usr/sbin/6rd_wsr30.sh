#!/bin/sh
# 6rd.sh - IPv6-in-IPv4 tunnel backend
# Copyright (c) 2010-2012 OpenWrt.org

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

proto_6rd_setup() {
	local cfg="$1"
	local iface="$2"
	local link="6rd-$cfg"

	local ck6rdEnable=$(uci get network.general."$cfg"_enable)
	[ "$ck6rdEnable" == "0" ] && return

	local defaultroute
	config_get_bool defaultroute "$cfg" defaultroute 1

	local ipaddr
	# The ipaddr guesses local IPv4 address from the
	# interface used by the default route.
	[ -z "$ipaddr" ] && {
		for i in $(seq 1 5);
		do
			sleep 1
			local wanif=$(find_6in4_wanif)
			#echo "== i=$i wanif=$wanif ===" #> /dev/console
			[ -n "$wanif" ] && {
				ipaddr=$(find_6in4_wanip "$wanif")
				#echo "== ipaddr_2=$ipaddr ===" #> /dev/console
				uci_set_state network "$cfg" wan_device "$wanif"
				break
			}
		done
	}

	local iface6rd=$(uci_get network wan iface6rd)
	# local reqopts=$(uci_get network wan reqopts)

	local peeraddr
	local ip4prefixlen
	local ip6prefix
	local ip6prefixlen

	[ -n "$iface6rd" -a "$iface6rd" == "wan6rd" ] && {
		[ -f /var/log/udhcpc_ip6rd.log ] && {
			local ip6rds=$(cat /var/log/udhcpc_ip6rd.log)
			#echo "=== ip6rds=$ip6rds ===" #> /dev/console
			ip4prefixlen="${ip6rds%% *}"
			ip6rds="${ip6rds#* }"
			ip6prefixlen="${ip6rds%% *}"
			ip6rds="${ip6rds#* }"
			ip6prefix="${ip6rds%% *}"
			ip6rds="${ip6rds#* }"
			peeraddr="${ip6rds%% *}"
			#echo "=== ip4prefixlen=$ip4prefixlen ip6prefix=$ip6prefix peeraddr=$peeraddr ===" #> /dev/console
			rm /var/log/udhcpc_ip6rd.log
		} || {
			echo "Cannot find out /var/log/udhcpc_ip6rd.log file - skipping"
			return
		}
	} || {
		peeraddr=$(uci_get network "$cfg" peeraddr)
		ip4prefixlen=$(uci_get network "$cfg" ip4prefixlen)
		ip6prefix=$(uci_get network "$cfg" ip6prefix)
		ip6prefixlen=$(uci_get network "$cfg" ip6prefixlen)
	}

	[ -z "$ip6prefix" -o -z "$peeraddr" ] && {
		echo "Cannot determine IPv6 prefix or peer for 6rd tunnel $cfg - skipping" #> /dev/console
		return
	}

	[ -n "$ipaddr" ] && {
		# Determine the relay prefix.
		local ip4prefixlen="${ip4prefixlen:-0}"
		local ip4prefix=$(sh /usr/sbin/ipcalc.sh "$ipaddr/$ip4prefixlen" | grep NETWORK)
		ip4prefix="${ip4prefix#NETWORK=}"

		#local ip4prefix_0=$(ipcalc.sh "$ipaddr/$ip4prefixlen")
		#echo "=== ip4prefix_0=$ip4prefix_0 ===" #> /dev/console

		#echo "=== ipaddr=$ipaddr ===" #> /dev/console
		#echo "=== ip4prefix=$ip4prefix ===" #> /dev/console
		#echo "=== ip4prefixlen=$ip4prefixlen ===" #> /dev/console

		# Determine our IPv6 address.
		local ip6subnet=$(6rdcalc "$ip6prefix/$ip6prefixlen" "$ipaddr/$ip4prefixlen")
		local ip6addr="${ip6subnet%%::*}::1"

		#echo "== ip6subnet=$ip6subnet , 6rdcalc $ip6prefix/$ip6prefixlen $ipaddr/$ip4prefixlen ==" > /dev/console

		#echo "=== ip6addr=$ip6addr ===" #> /dev/console
		#echo "=== ip6prefix=$ip6prefix ===" #> /dev/console
		#echo "=== ip6prefixlen=$ip6prefixlen ===" #> /dev/console

		#echo "=== peeraddr=$peeraddr ===" #> /dev/console

		# creating the tunnel below will trigger a net subsystem event
		# prevent it from touching or iface by disabling .auto here
		uci_set_state network "$cfg" ifname $link
		uci_set_state network "$cfg" auto 0

		ip tunnel add $link mode sit remote any local $ipaddr ttl ${ttl:-64}
		ip link set $link up
		sleep 1
		ip link set mtu ${mtu:-1280} dev $link
		ip addr add $ip6addr dev $link

		[ "$defaultroute" = 1 ] && {
			#echo "=== login default route ===" #> /dev/console
			# Change default route
			ip -6 route change via ::$peeraddr dev $link
			uci_set_state network "$cfg" defaultroute 1
		}

		echo ::$peeraddr >/tmp/6rd_route
	} || {
		echo "Cannot determine local IPv4 address for 6rd tunnel $cfg - skipping" #> /dev/console
		return
	}

	##Add for assign LAN ipv6 address
	local ipv6lan=
	local ipv6lanold=$(uci get network."$cfg".zyipv6lan)
	local oldIpv6Subnet=$(uci get network."$cfg".zyPd6rd)
	local ipv6lanprefix="${ip6subnet%%::*}"

	local product_name=$(uci get system.main.product_name)
	local config_section=$(echo "$cfg"| sed s/6rd//g)
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

	##add 6rd CPE lan-ipv6 
	local mac=$(ifconfig $lanIface | sed -ne 's/[[:space:]]*$//; s/.*HWaddr //p')
	ipv6lan=$ipv6lanprefix:$(printf %02x $((0x${mac%%:*} ^ 2)))
	mac=${mac#*:}
	ipv6lan=$ipv6lan${mac%:*:*:*}ff:fe
	mac=${mac#*:*:}
	ipv6lan=$ipv6lan${mac%:*}${mac##*:}

	[ -n "$ipv6lanold" -a "$ipv6lanold" != "$ipv6lan" ] && ifconfig $lanIface del "$ipv6lanold"/64
	ifconfig $lanIface add "$ipv6lan"/64

	uci set network."$cfg".zyipv6lan=$ipv6lan
	uci set network."$cfg".zyPd6rd=$ip6subnet
	uci commit network	

	echo 1 > /tmp/radvd_6rd

	/etc/init.d/radvd restart	
	##END
}

stop_interface_6rd() {
	#echo "== 6rd_stop 1 ===" #> /dev/console
	local cfg="$1"
	local link="6rd-$cfg"

	local defaultroute=$(uci_get_state network "$cfg" defaultroute)

	grep -qs "^ *$link:" /proc/net/dev && {
		#echo "== 6rd_stop 2 ===" #> /dev/console

		[ "$defaultroute" = "1" ] && {
			ip -6 route del dev $link
		}

		ip link set $link down
		ip tunnel del $link
	}
}

[ -n "$INCLUDE_ONLY" ] || {
	[ -f /lib/netifd/proto/6rd ] || {
		stop_interface_6rd wan6rd
	}
	proto_6rd_setup wan6rd
}
