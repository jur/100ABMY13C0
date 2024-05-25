#!/bin/sh

WAN=$(uci get network.wan.iface)

boot(){
	# remove parental control chain in FORWARD chain
	iptables -D FORWARD -j parental_keyword
	iptables -D FORWARD -j parental_network_service
	iptables -D FORWARD -j parental_schedule

	# delete chain
	iptables -X parental_keyword
	iptables -X parental_network_service
	iptables -X parental_schedule

	# create chain
	iptables -N parental_keyword
	iptables -N parental_network_service
	iptables -N parental_schedule

	# add parental control chain to FORWARD chain
	iptables -I FORWARD -j parental_keyword
	iptables -I FORWARD -j parental_network_service
	iptables -I FORWARD -j parental_schedule

	# configure parental_routing chain
	iptables -t nat -D PREROUTING -j prerouting_parental_rule
	iptables -t nat -D PREROUTING -j parental_routing
	iptables -t nat -X prerouting_parental_rule
	iptables -t nat -X parental_routing
	iptables -t nat -N prerouting_parental_rule
	iptables -t nat -N parental_routing
	iptables -t nat -A PREROUTING -j prerouting_parental_rule
	iptables -t nat -A PREROUTING -j parental_routing
}

clean(){
	# clean chain rule
	iptables -F parental_keyword
	iptables -F parental_network_service
	iptables -F parental_schedule
	iptables -t nat -F parental_routing
	iptables -t nat -F prerouting_parental_rule
}

check(){
	keyword_chain=$(iptables -vnL parental_keyword)
	network_service_chain=$(iptables -vnL parental_network_service)
	schedule_chain=$(iptables -vnL parental_schedule)

	if [ -z "$keyword_chain" ] || [ -z "$network_service_chain" ] || [ -z "$schedule_chain" ]; then
		echo -n "chainNotExist"
		return
	fi
	echo -n "chainExist"
}

bonus(){
	ruleNum="$2"
	minute="$3"
	ruleComment="rule$ruleNum"

	mac_list=$(uci get parentalControl.$ruleComment.maclist)
	echo "#!/bin/sh" >> /tmp/parentalBonus"$ruleNum"
	for mac in $mac_list
	do
		iptables -I parental_schedule -m mac --mac-source $mac -m comment --comment $ruleComment -j RETURN
		echo "iptables -D parental_schedule -m mac --mac-source $mac -m comment --comment $ruleComment -j RETURN" >> /tmp/parentalBonus"$ruleNum"
		iptables -t nat -I parental_routing -m mac --mac-source $mac -m comment --comment $ruleComment -j RETURN
		echo "iptables -t nat -D parental_routing -m mac --mac-source $mac -m comment --comment $ruleComment -j RETURN" >> /tmp/parentalBonus"$ruleNum"
	done

	echo "uci set parentalControl.$ruleComment.timestamp=0" >> /tmp/parentalBonus"$ruleNum"
	echo "uci commit parentalControl" >> /tmp/parentalBonus"$ruleNum"
	echo "rm -- /tmp/parentalBonus$ruleNum" >> /tmp/parentalBonus"$ruleNum"

	chmod +x /tmp/parentalBonus"$ruleNum"

	at now + $minute minutes -f /tmp/parentalBonus"$ruleNum"
}


immediateAction() {
	ruleNum="$2"
	timestamp="$3"
	action="$4"
	ruleComment="rule$ruleNum"
	mac_list=$(uci get parentalControl."$ruleComment".maclist)
	if [ -f /tmp/parentalImmediateAction"$ruleNum" ]; then
		while IFS= read -r LINE
		do
			string=$(echo "$LINE" | grep iptables)
			eval "$string"
			string=$(echo "$LINE" | grep atrm)
			eval "$string"
			string=$(echo "$LINE" | grep 'rm --')
			eval "$string"
		done < /tmp/parentalImmediateAction"$ruleNum"
	fi

	## date command option " -d @... " does not work
	# local at_timeformat=$(date -d @"$timestamp" '+%H:%M:00 %Y')
	local at_timeformat=$(date -D '%s' -d "$timestamp" '+%H:%M:00 %Y')
	local delay=$(at -l | grep -c "$at_timeformat") #if we execute multiple ImmediateAction scripts at the same time, the iptables command maybe unable to be handle.
	echo "#!/bin/sh" > /tmp/parentalImmediateAction"$ruleNum"
	if [ "$delay" -gt 0 ] && [ "$timestamp" != 0 ]; then
		echo "sleep $delay" >> /tmp/parentalImmediateAction"$ruleNum"
	fi

	for mac in $mac_list
	do
		iptables -I parental_schedule -m mac --mac-source $mac -m comment --comment "$ruleComment immediateAction" -j $action
		echo "iptables -D parental_schedule -m mac --mac-source $mac -m comment --comment \"$ruleComment immediateAction\" -j $action" >> /tmp/parentalImmediateAction"$ruleNum"
		if [ "$action" = "RETURN" ]; then
			iptables -t nat -I parental_routing -m mac --mac-source $mac -m comment --comment "$ruleComment immediateAction" -j $action
			echo "iptables -t nat -D parental_routing -m mac --mac-source $mac -m comment --comment \"$ruleComment immediateAction\" -j $action" >> /tmp/parentalImmediateAction"$ruleNum"
		elif [ "$action" = "REJECT" ]; then
			local lan_IP=$(uci get network.br0.ipaddr)
			local redirect_enable=$(uci get parentalControl.general.enable_redirect)
			local redirect_https_port=$(uci get parentalControl.general.redirect_HTTPS_port)

			if [ "$redirect_enable" == 1 ]; then
				iptables -t nat -I parental_routing ! -i $WAN ! -d $lan_IP -p tcp --dport 80 -m mac --mac-source $mac -j DNAT --to $lan_IP:8008
				echo "iptables -t nat -D parental_routing ! -i $WAN ! -d $lan_IP -p tcp --dport 80 -m mac --mac-source $mac -j DNAT --to $lan_IP:8008" >> /tmp/parentalImmediateAction"$ruleNum"
				iptables -t nat -I parental_routing ! -i $WAN ! -d $lan_IP -p tcp --dport 443 -m mac --mac-source $mac -j DNAT --to $lan_IP:$redirect_https_port
				echo "iptables -t nat -D parental_routing ! -i $WAN ! -d $lan_IP -p tcp --dport 443 -m mac --mac-source $mac -j DNAT --to $lan_IP:$redirect_https_port" >> /tmp/parentalImmediateAction"$ruleNum"
			fi

			parental_tool -t clean_session -m $mac &
			sleep 1
			parental_tool -t clean_session -m $mac &
		fi
	done
	# Finally, parentalControl config reset to default when immediate action finish.
	{
		echo "uci set parentalControl.$ruleComment.immediate_block_state=\"\""
		echo "uci set parentalControl.$ruleComment.immediate_action_stoptimestamps=0"
		echo "uci commit parentalControl"
		[ "$action" = "RETURN" ] && echo "parental_tool -t clean_rule_session_follow_schedule -r $ruleNum &" && echo "sleep 1" && echo "parental_tool -t clean_rule_session_follow_schedule -r $ruleNum &"
	} >> /tmp/parentalImmediateAction"$ruleNum"
	## ==
	echo "rm -- /tmp/parentalImmediateAction$ruleNum" >> /tmp/parentalImmediateAction"$ruleNum"
	chmod +x /tmp/parentalImmediateAction"$ruleNum"

	if [ "$timestamp" != 0 ] ; then #if timestamp = 0 ; reject/return for forever
		#stoptime=$(date -d @"$timestamp" '+%H:%M %Y-%m-%d')
		stoptime=$(date -D '%s' -d "$timestamp" '+%H:%M %Y-%m-%d')
		at "$stoptime" -f /tmp/parentalImmediateAction"$ruleNum"
		echo "atrm $(at -l | awk 'BEGIN{FS=" "} {print $1}' | sed -n '1p')" >> /tmp/parentalImmediateAction"$ruleNum"
	fi
}

date -k

case "$1" in
	boot )
		boot
		;;
	clean )
		clean
		;;
	check )
		check
		;;
	bonus )
		bonus "$@"
		;;
	immediateAction )
	  immediateAction "$@"
		;;
esac
