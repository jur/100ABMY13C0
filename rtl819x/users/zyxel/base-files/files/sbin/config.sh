#!/bin/sh
# Copyright (C) 2006 OpenWrt.org

do_sysctl() {
	[ -n "$2" ] && \
		sysctl -n -e -w "$1=$2" >/dev/null || \
		sysctl -n -e "$1"
}

map_sysctls() {
	local cfg="$1"
	local ifn="$2"

	local fam
	for fam in ipv4 ipv6; do
		if [ -d /proc/sys/net/$fam ]; then
			local key
			for key in /proc/sys/net/$fam/*/$ifn/*; do
				local val
				config_get val "$cfg" "${fam}_${key##*/}"
				[ -n "$val" ] && echo -n "$val" > "$key"
			done
		fi
	done
}

find_config() {
	local iftype device iface ifaces ifn
	for ifn in $interfaces; do
		config_get iftype "$ifn" type
		config_get iface "$ifn" ifname
		case "$iftype" in
			bridge) config_get ifaces "$ifn" ifnames;;
		esac
		config_get device "$ifn" device
		for ifc in $device $iface $ifaces; do
			[ ."$ifc" = ."$1" ] && {
				echo "$ifn"
				return 0
			}
		done
	done

	return 1;
}

scan_interfaces() {
	local cfgfile="${1:-network}"
	interfaces=
	config_cb() {
		case "$1" in
			interface)
				config_set "$2" auto 1
			;;
		esac
		local iftype ifname device proto
		config_get iftype "$CONFIG_SECTION" TYPE
		case "$iftype" in
			interface)
				append interfaces "$CONFIG_SECTION"
				config_get proto "$CONFIG_SECTION" proto
				config_get iftype "$CONFIG_SECTION" type
				config_get ifname "$CONFIG_SECTION" ifname
				config_get device "$CONFIG_SECTION" device "$ifname"
				config_set "$CONFIG_SECTION" device "$device"
				case "$iftype" in
					bridge)
						config_set "$CONFIG_SECTION" ifnames "$device"
						config_set "$CONFIG_SECTION" ifname "$CONFIG_SECTION"
					;;
				esac
				( type "scan_$proto" ) >/dev/null 2>/dev/null && eval "scan_$proto '$CONFIG_SECTION'"
			;;
		esac
	}
	config_load "${cfgfile}"
}

add_vlan() {
	local vif="${1%\.*}"

	[ "$1" = "$vif" ] || ifconfig "$1" >/dev/null 2>/dev/null || {
		ifconfig "$vif" up 2>/dev/null >/dev/null || add_vlan "$vif"
		$DEBUG vconfig add "$vif" "${1##*\.}"
		return 0
	}
	return 1
}

# add dns entries if they are not in resolv.conf yet
add_dns() {
	local cfg="$1"; shift

	remove_dns "$cfg"

	# We may be called by pppd's ip-up which has a nonstandard umask set.
	# Create an empty file here and force its permission to 0644, otherwise
	# dnsmasq will not be able to re-read the resolv.conf.auto .
	[ ! -f /var/resolv.conf ] && {
		touch /var/resolv.conf
		chmod 0644 /var/resolv.conf
	} || {
		rm /var/resolv.conf
	}

	local dns
	local add
	local service_type

	for dns in "$@"; do
		grep -qsE "^nameserver ${dns//./\\.}$" /var/resolv.conf || {
		add="${add:+$add }$dns"
		echo "nameserver $dns" >> /var/resolv.conf
		}
	done

	# The info are from ISP
	[ -f /var/resolv.conf ] && {
		cp /var/resolv.conf /var/resolv_from_ISP.conf.auto
		rm /var/resolv.conf
		touch /var/resolv.conf
		chmod 0644 /var/resolv.conf
	}
	
	grep -qsE "^nameserver ${dns//./\\.}$" /var/resolv.conf || {
		add="${add:+$add }$dns"
	}

	local j=1;
	for dns in "$@"; do
		# Filter 'User define' and 'None' condition out.
		service_type=$(uci get network.$cfg.dns$j)
		service_types=$(echo $service_type | sed 's/USER,.*$//g')
		if [ $service_types != "USER," ] && [ $service_types != "NONE," ]; then        
			# add for save dns to wan uci module
			uci set network.$cfg.dns$j="ISP,$dns"
		else
			service_types=$(echo $service_type | awk -F "," '{print $1}')
			if [ $service_types == "USER" ]; then        
				dns=$(echo $service_type | awk -F "," '{print $2}')
			fi
			if [ $service_types == "NONE" ]; then
				dns="0.0.0.0"
			fi
		fi		
		if [ $dns != "0.0.0.0" ]; then
			echo "nameserver $dns" >> /var/resolv.conf
		fi
		j=$((j+1))
	done

	[ -n "$cfg" ] && {
        	uci_toggle_state network "$cfg" dns "$add"
	}
}

# remove dns entries of the given iface
remove_dns() {
	local cfg="$1"

	[ -n "$cfg" ] && {
		[ "$cfg" = "br0" ] && return 0

		[ "$cfg" = "wan" ] && {
			local dnrdpid=$(pidof dnrd)
			[ -n "$dnrdpid" ] && service_kill dnrd "$dnrdpid"
		}

		[ -f /var/resolv.conf ] && {
			local dns=$(uci_get_state network "$cfg" resolv_dns)
			[ -n "$dns" ] && { 
				for dns in $dns; do
					sed -i -e "/^nameserver ${dns//./\\.}$/d" /var/resolv.conf
				done
			} || {
				rm /var/resolv.conf
				[ -f /etc/udhcpc/resolv.conf ] && rm /etc/udhcpc/resolv.conf
				touch /var/resolv.conf
			}
		}

		[ -d /var/state ] && {
			uci_revert_state network "$cfg" dns
			uci_revert_state network "$cfg" resolv_dns
		}
	}
}

reload_dns() {
		lock /tmp/.reload_dns.lock
        local cfg="$1"
        local config="$2"		

        #[ -f /var/resolv.conf ] || return 0
		if [ -f /var/resolv.conf ]; then
			echo "start reload dns......."
		else 
			lock -u /tmp/.reload_dns.lock
			return 0
		fi 

        local dns
        local dns_fromISP
        local service_type
        local service_types
        local orign_resolv="/var/resolv.conf"

        if [ $cfg == "wan" ]; then
          local wan_proto=$(uci get network.wan.proto)
          if [ $wan_proto == "dhcp" ]; then
             [ -f /var/udhcpc/resolv.conf ] && {
                 local udhcpc_dns=$(cat /var/udhcpc/resolv.conf)
                 local var_dns=$(cat /var/resolv.conf)
                 if [ "$udhcpc_dns" != "$var_dns" ]; then
                    mv /var/resolv.conf /var/resolv_not_sync.conf
                    cp /var/udhcpc/resolv.conf /var/resolv.conf
                 fi
             }
          fi
          if [ $wan_proto == "pppoe" ]; then
             [ -f /var/ppp/resolv.conf ] && {
                 local ppp_dns=$(cat /var/ppp/resolv.conf)
                 local var_dns=$(cat /var/resolv.conf)
                 if [ "$ppp_dns" != "$var_dns" ]; then
                    mv /var/resolv.conf /var/resolv_not_sync.conf
                    cp /var/ppp/resolv.conf /var/resolv.conf
                 fi
             }
          fi
          cp /var/resolv.conf /var/resolv_from_ISP.conf.auto
          rm /var/resolv.conf
          touch /var/resolv.conf
          orign_resolv="/var/resolv_from_ISP.conf.auto"
        fi

        while read line; do
          dns=$(echo "$line" | grep -v '^#' | grep nameserver | awk '{print $2}')
          [ -n "$dns" ] && {
            [ -z "$dns_fromISP" ] && {
              dns_fromISP="$dns"
            } || {
              dns_fromISP="$dns_fromISP $dns"
            }
          }
        done < $orign_resolv
		
        local j=1;
        for dns in $dns_fromISP; do
                # Filter 'User define' and 'None' condition out.
                service_type=$(uci get network.$cfg.dns$j)
                service_types=$(echo $service_type | awk -F "," '{print $1}')
                if [ $service_types == "ISP" ]; then
                    # save dns when network's wan option is ISP, the dns is from ISP.
                    uci set network.$cfg.dns$j="ISP,$dns"
                else
                    if [ $service_types == "USER" ]; then
                        dns=$(echo $service_type | awk -F "," '{print $2}')
                    fi
                    if [ $service_types == "NONE" ]; then
                        dns="0.0.0.0"
                    fi
                fi
                if [ $cfg == "wan" ] && [ $dns != "0.0.0.0" ]; then
                        echo "nameserver $dns" >> /var/resolv.conf
                fi
                j=$((j+1))
        done

        if [ $j -le 3 ]; then
           local i=$j;
           while [ "$i" != "4" ]
           do
             service_type=$(uci get network.$cfg.dns$i)
             service_types=$(echo $service_type | awk -F "," '{print $1}')
             if [ $service_types == "ISP" ]; then
                uci set network.$cfg.dns$i="ISP,"
             fi
             i=$((i+1))
           done
        fi

        uci_commit $config
		echo "end reload dns........"
		lock -u /tmp/.reload_dns.lock
}

# sort the device list, drop duplicates
sort_list() {
	local arg="$*"
	(
		for item in $arg; do
			echo "$item"
		done
	) | sort -u
}

# Create the interface, if necessary.
# Return status 0 indicates that the setup_interface() call should continue
# Return status 1 means that everything is set up already.

prepare_interface() {
	local iface="$1"
	local config="$2"
	local vifmac="$3"

	# if we're called for the bridge interface itself, don't bother trying
	# to create any interfaces here. The scripts have already done that, otherwise
	# the bridge interface wouldn't exist.
	[ "$config" = "$iface" -o -e "$iface" ] && return 0;

	ifconfig "$iface" 2>/dev/null >/dev/null && {
		local proto
		config_get proto "$config" proto

		# make sure the interface is removed from any existing bridge and deconfigured,
		# (deconfigured only if the interface is not set to proto=none)
		unbridge "$iface"
		[ "$proto" = none ] || ifconfig "$iface" 0.0.0.0

		# Change interface MAC address if requested
		[ -n "$vifmac" ] && {
			ifconfig "$iface" down
			ifconfig "$iface" hw ether "$vifmac" up
		}

		# Apply sysctl settings
		map_sysctls "$config" "$iface"
	}

	# Setup VLAN interfaces
	add_vlan "$iface" && return 1
	ifconfig "$iface" 2>/dev/null >/dev/null || return 0

	# Setup bridging
	local iftype
	config_get iftype "$config" type
	case "$iftype" in
		bridge)
			local macaddr
			config_get macaddr "$config" macaddr
			[ -x /bin/brctl ] && {
				ifconfig "$config" 2>/dev/null >/dev/null && {
					local newdevs devices
					config_get devices "$config" device
					for dev in $(sort_list "$devices" "$iface"); do
						append newdevs "$dev"
					done
					uci_toggle_state network "$config" device "$newdevs"
					$DEBUG ifconfig "$iface" 0.0.0.0
					$DEBUG do_sysctl "net.ipv6.conf.$iface.disable_ipv6" 1
					$DEBUG brctl addif "$config" "$iface"
					# Bridge existed already. No further processing necesary
				} || {
					local stp
					config_get_bool stp "$config" stp 0
					$DEBUG brctl addbr "$config"
					$DEBUG brctl setfd "$config" 0
					$DEBUG ifconfig "$iface" 0.0.0.0
					$DEBUG do_sysctl "net.ipv6.conf.$iface.disable_ipv6" 1
					$DEBUG brctl addif "$config" "$iface"
					$DEBUG brctl stp "$config" $stp
					[ -z "$macaddr" -a -e "/sys/class/net/$iface/address" ] && macaddr="$(cat /sys/class/net/$iface/address)"
					$DEBUG ifconfig "$config" ${macaddr:+hw ether "${macaddr}"} up
				}
				ifconfig "$iface" ${macaddr:+hw ether "${macaddr}"} up 2>/dev/null >/dev/null
				ifconfig "$config" 2>/dev/null >/dev/null && { 
					[ "$config" == "br0" ] && return 0
				}
				return 1
			}
		;;
	esac
	return 0
}

set_interface_ifname() {
	local config="$1"
	local ifname="$2"

	local device
	config_get device "$1" device
	uci_toggle_state network "$config" ifname "$ifname"
	uci_toggle_state network "$config" device "$device"
}

setup_interface_none() {
	env -i ACTION="ifup" INTERFACE="$2" DEVICE="$1" PROTO=none
}

setup_interface_static() {
	local iface="$1"
	local config="$2"

	stop_interface_all "$config"
	[ "$config" == "br0" ] && iface="br0"

	local ipaddr netmask ip6addr
	config_get ipaddr "$config" ipaddr
	config_get netmask "$config" netmask
	config_get ip6addr "$config" ip6addr
	[ -z "$ipaddr" -o -z "$netmask" ] && [ -z "$ip6addr" ] && return 1

	local gateway ip6gw dns bcast metric
	config_get gateway "$config" gateway
	config_get ip6gw "$config" ip6gw
	config_get dns "$config" dns
	config_get dns1 "$config" dns1
	config_get bcast "$config" broadcast
	config_get metric "$config" metric

	case "$ip6addr" in
		*/*) ;;
		*:*) ip6addr="$ip6addr/64" ;;
	esac

	[ -z "$ipaddr" ] || $DEBUG ifconfig "$iface" "$ipaddr" netmask "$netmask" broadcast "$bcast"
	[ -z "$ip6addr" ] || $DEBUG ifconfig "${iface%:*}" add "$ip6addr"
	[ -z "$gateway" ] || $DEBUG route add default gw "$gateway" ${metric:+metric $metric} dev "$iface"
	[ -z "$ip6gw" ] || $DEBUG route -A inet6 add default gw "$ip6gw" ${metric:+metric $metric} dev "${iface%:*}"
	[ -z "$dns" ] || add_dns "$config" "$dns"
	[ -z "$dns1" ] || {
		[ "$config" == "br0" ] || add_dns "$config" "$dns"
	}

	config_get type "$config" TYPE
	[ "$type" = "alias" ] && return 0
	
	[ "$iface" == "br0" ] && iface="$1"

	env -i ACTION="ifup" INTERFACE="$config" DEVICE="$iface" PROTO=static
}

setup_interface_alias() {
	local config="$1"
	local parent="$2"
	local iface="$3"

	local cfg
	config_get cfg "$config" interface
	[ "$parent" == "$cfg" ] || return 0

	# parent device and ifname
	local p_device p_type
	config_get p_device "$cfg" device
	config_get p_type   "$cfg" type

	# select alias ifname
	local layer use_iface
	config_get layer "$config" layer 2
	case "$layer:$p_type" in
		# layer 3: e.g. pppoe-wan or pptp-vpn
		3:*)      use_iface="$iface" ;;

		# layer 2 and parent is bridge: e.g. wan
		2:bridge) use_iface="$cfg" ;;

		# layer 1: e.g. eth0 or ath0
		*)        use_iface="$p_device" ;;
	esac

	# alias counter
	local ctr
	config_get ctr "$parent" alias_count 0
	ctr="$(($ctr + 1))"
	config_set "$parent" alias_count "$ctr"

	# alias list
	local list
	config_get list "$parent" aliases
	append list "$config"
	config_set "$parent" aliases "$list"

	use_iface="$use_iface:$ctr"
	set_interface_ifname "$config" "$use_iface"

	local proto
	config_get proto "$config" proto "static"
	case "${proto}" in
		static)
			setup_interface_static "$use_iface" "$config"
		;;
		*)
			echo "Unsupported type '$proto' for alias config '$config'"
			return 1
		;;
	esac
}

setup_interface() {
	local iface="$1"
	local config="$2"
	local proto="$3"
	local vifmac="$4"

	[ -n "$config" ] || {
		config=$(find_config "$iface")
		[ "$?" = 0 ] || return 1
	}

	prepare_interface "$iface" "$config" "$vifmac" || return 0

	[ "$iface" = "$config" ] && {
		# need to bring up the bridge and wait a second for
		# it to switch to the 'forwarding' state, otherwise
		# it will lose its routes...
		ifconfig "$iface" up
		sleep 1
	}

	# Interface settings
	grep -qE "^ *$iface:" /proc/net/dev && {
		local mtu macaddr
		config_get mtu "$config" eth_mtu
		config_get macaddr "$config" macaddr
		[ -n "$macaddr" ] && $DEBUG ifconfig "$iface" down
		$DEBUG ifconfig "$iface" ${macaddr:+hw ether "$macaddr"} ${mtu:+mtu $mtu} up
	}
	set_interface_ifname "$config" "$iface"

	if [ $config == "br0" ] && [ $iface == "eth1" ]; then
		GETMIB="flash get"
		eval `$GETMIB HW_NIC0_ADDR`
		ifconfig $config hw ether $HW_NIC0_ADDR
	fi 

	[ -n "$proto" ] || config_get proto "$config" proto
	case "$proto" in
		static)
			setup_interface_static "$iface" "$config"
		;;
		dhcp)
			config_get wan_ifname wan ifname
			if [ $wan_ifname == $iface ]; then
				if [ $config != "wan" ]; then
					config_get wan_proto wan proto
					if [ $wan_proto == "dhcp" ]; then
						stop_interface_dhcp wan
					fi
					if [ $wan_proto == "pppoe" ] || [ $wan_proto == "pptp" ]; then
						stop_interface_ppp_zy wan
					fi
					iface="br0"
				fi
			fi
			if [ $iface == "eth0" ]; then
				iface="br0"
			fi
			# kill running udhcpc instance
			local pidfile="/var/udhcpc/udhcpc-${iface}.pid"
			local dhcpshfile="/usr/share/udhcpc/${iface}.sh"
			service_kill udhcpc "$pidfile"
			[ -e "/var/resolv.conf" ] && rm /var/resolv.conf

			local ipaddr netmask hostname proto1 clientid vendorid broadcast reqopts
			config_get ipaddr "$config" ipaddr
			config_get netmask "$config" netmask
			if [ $config != "br0" ]; then
				hostname=`uci get system.main.hostname`
			fi
			config_get proto1 "$config" proto
			config_get clientid "$config" clientid
			config_get vendorid "$config" vendorid
			config_get reqopts "$config" reqopts
			config_get_bool broadcast "$config" broadcast 0

			[ -z "$ipaddr" ] || \
				$DEBUG ifconfig "$iface" "$ipaddr" ${netmask:+netmask "$netmask"}

			# don't stay running in background if dhcp is not the main proto on the interface (e.g. when using pptp)
			local dhcpopts
			#[ ."$proto1" != ."$proto" ] && dhcpopts="-n -q"
			[ "$broadcast" = 1 ] && broadcast="-O broadcast" || broadcast=

			$DEBUG eval udhcpc -i "$iface" \
				${hostname:+-H $hostname} \
				${clientid:+-c $clientid} \
				${vendorid:+-V $vendorid} \
				-b -p "$pidfile" -s "$dhcpshfile" \
				${reqopts:+-O $reqopts} ${hostname:--q}
		;;
		none)
			setup_interface_none "$iface" "$config"
		;;
		*)
			if ( eval "type setup_interface_$proto" ) >/dev/null 2>/dev/null; then
				eval "setup_interface_$proto '$iface' '$config' '$proto'"
			else
				echo "Interface type $proto not supported."
				return 1
			fi
		;;
	esac
}

stop_interface_dhcp() {
	local config="$1"

	local ifname
	config_get ifname "$config" ifname

	remove_dns "$config"

	local pidfile="/var/udhcpc/udhcpc-${ifname}.pid"
	service_kill udhcpc "$pidfile"

	[ -d /var/state ] && {
		uci -P /var/state revert "network.$config"
	}
}

stop_interface_ppp_zy() {
	local config="$1"
	local ppp_proto="ppp0 pptp"

	local pppd_pid=$(pidof pppd)
	[ -n "$pppd_pid" ] && service_kill pppd "$pppd_pid"
	
	for proto in $ppp_proto; do
		local link="$proto"

		# kill active ppp daemon and other processes
		local pids="$(head -n1 -q /var/ppp/link /var/run/${link}.pid 2>/dev/null)"
		for pid in $pids; do 
			[ -d "/proc/$pid" ] && {
				kill $pid
				[ -d "/proc/$pid" ] && {
					sleep 1
					kill -9 $pid 2>/dev/null >/dev/null
				}
			}
		done
	done

	[ -f "/var/ppp/link" ] && rm /var/ppp/link

	[ -d /var/state ] && {
		uci -P /var/state revert "network.$config"
	}
}

stop_interface_all(){
	local config="$1"

	##stop wan interface handler
	stop_interface_dhcp "$config"
	stop_interface_pppoe "$config"
	stop_interface_ppp_zy "$config"
}

unbridge() {
	local dev="$1"
	local brdev

	[ -x /bin/brctl ] || return 0
	brctl show 2>/dev/null | grep "$dev" >/dev/null && {
		# interface is still part of a bridge, correct that

		for brdev in $(brctl show | awk '$2 ~ /^[0-9].*\./ { print $1 }'); do
			brctl delif "$brdev" "$dev" 2>/dev/null >/dev/null
			do_sysctl "net.ipv6.conf.$dev.disable_ipv6" 0
		done
	}

	config_get wan_ifname wan ifname
	brctl show 2>/dev/null | grep "$wan_ifname" >/dev/null && {
		if [ $brdev == "br0" ]; then
			GETMIB="flash get"
			eval `$GETMIB OP_MODE`
			[ "$OP_MODE" == "0" ] && {
				brctl delif "$brdev" "$wan_ifname" 2>/dev/null >/dev/null
			}
		fi
	}	
}
