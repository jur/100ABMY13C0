stop_interface_pppoe() {
	local cfg="$1"

	local pppd_pid=$(pidof pppd)
	[ -n "$pppd_pid" ] && service_kill pppd "$pppd_pid"

	local link="ppp0"

	# kill active ppp daemon and other processes
	local pids="$(head -n1 -q /var/ppp/link /var/run/${link}.pid 2>/dev/null)"

	service_kill pppd "/var/ppp/link"

	remove_dns "$cfg"
	[ -f /var/ppp/resolv.conf ] && rm /var/ppp/resolv.conf
	
	[ -f "/var/ppp/link" ] && rm /var/ppp/link
	[ -f "/var/ppp/options" ] && rm /var/ppp/options
	[ -f "/var/ppp/chap-secrets" ] && rm /var/ppp/chap-secrets
	[ -f "/var/ppp/pap-secrets" ] && rm /var/ppp/pap-secrets
	[ -f "/var/ppp/firewall_lock" ] && rm /var/ppp/firewall_lock	
}

setup_interface_pppoe() {
	local iface="$1"
	local config="$2"

	local mtu
	config_get mtu "$config" mtu
	mtu=${mtu:-1492}

	local ac
	config_get ac "$config" ac

	local username
	config_get username "$config" username

	local password
	config_get password "$config" password
	
	local service
	config_get service "$config" pppoeServiceName
	[ -n "$service" ] && {
		local pppoeService="rp_pppoe_service $service"
		service=$pppoeService
	}

	cat <<EOF > /var/ppp/options
name "$username"
noauth
noccp
nomppc
noipdefault
hide-password
defaultroute
persist
ipcp-accept-remote
ipcp-accept-local
nodetach
usepeerdns
+ipv6
mtu $mtu
mru $mtu
lcp-echo-interval 20
lcp-echo-failure 3
holdoff 3
maxfail 0
plugin /usr/lib/pppd/rp-pppoe.so $service eth1
EOF

	local pppoeWanIpAddr
	config_get pppoeWanIpAddr "$config" pppoeWanIpAddr
	[ -n "$pppoeWanIpAddr" ] && {
		local pppoeWanIpAddr_value="$pppoeWanIpAddr:"
		echo "$pppoeWanIpAddr_value" >> /var/ppp/options
	}

	local AutoConnect
	config_get AutoConnect "$config" pppoeNailedup
	if [ "$AutoConnect" == "0" ]; then
		local idles
		config_get idles "$config" demand
		local idle_time="idle $idles"
		echo "$idle_time" >> /var/ppp/options
	fi

	cat <<EOF > /var/ppp/chap-secrets
#################################################
"$username" *       "$password"
EOF

	cat <<EOF > /var/ppp/pap-secrets
#################################################
"$username" *       "$password"
EOF

	$DEBUG eval pppd &
}
