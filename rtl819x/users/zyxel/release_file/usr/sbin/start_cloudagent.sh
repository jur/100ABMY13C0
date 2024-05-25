#!/bin/sh

PKGPATH="/usr"

CLOUDAGENT_LOGGER="$PKGPATH/sbin/cloudagent_logger.sh"
LOCKFILE=/tmp/.init_cloudagent.lock

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/cloud_agent/lib

killall -9 zyxel_device_register
#killall -9 zyxel_xmpp_client
rm -rf /tmp/.cloudagent/agent_fifo
rm -rf /tmp/.cloudagent/cloud_credentials/credential_cache

# create cloud agent tmp dir
mkdir -p /tmp/.cloudagent

changeSite=$(uci get cloudagent.cloudagent.site_flag)
if [ "$changeSite" == "beta" ]; then
	cp /usr/share/zyxel_beta_cloud_agent_cert.conf -a /tmp/.cloudagent/zyxel_cloud_agent_cert.conf
	server_url=$(cat /usr/share/zyxel_beta_cloud_agent_configure.conf | awk -F'=' '{print $2}')
else
	cp /usr/share/zyxel_cloud_agent_cert.conf -a /tmp/.cloudagent/zyxel_cloud_agent_cert.conf
	server_url=$(cat /usr/share/zyxel_cloud_agent_configure.conf | awk -F'=' '{print $2}')
fi

# init bundle packages config
. /usr/sbin/init_bundle_config.sh
init_bundle_config $changeSite

# device will use this file to check the cloud domain is work to verify dns client is workable
echo "$server_url" | awk -F/ '{print $3}' > /tmp/.cloudagent/cloud_domain_internet_check

# cp $PKGPATH/share/zyxel_cloud_agent_cert.conf -a /tmp/.cloudagent
#echo $PKGPATH > /tmp/.cloudagent/agent_path
# server_url=$(cat ${PKGPATH}/share/zyxel_cloud_agent_configure.conf | awk -F'=' '{print $2}')
echo $server_url/user/1/token > /tmp/.cloudagent/validate_api
echo $server_url/d/3/register > /tmp/.cloudagent/device_register_api
echo $server_url/d/1/pairing > /tmp/.cloudagent/device_pair_api
echo $server_url/d/3/register/lite > /tmp/.cloudagent/device_findme_api
# WSQ50 not use findme
#echo "https://api-mycloud.zyxel.com/d/3/register/lite" > /tmp/.cloudagent/device_findme_api

restful_server_url=$(cat /tmp/.cloudagent/device_register_api)
restful_server_cert=$(cat /tmp/.cloudagent/zyxel_cloud_agent_cert.conf)

#generate a random number from 8~60
random_number=$(head /dev/urandom | hexdump -d | head -n 1 | cut -c 11-15)
sleep_seconds=$(expr $random_number % 53 + 8)
# unpair if necessary
do_unpair=$(uci get cloudagent.cloudagent.do_unpair)
if [ "$do_unpair" = "1" ];then
	while true; do
		if [ -z "$restful_server_cert" ];then
			unpair_result=$($PKGPATH/bin/zyxel_device_register -s $restful_server_url -r -c '1005')
		else
			unpair_result=$($PKGPATH/bin/zyxel_device_register -s $restful_server_url -r -c $restful_server_cert)
		fi
		unpair_success=$(echo $unpair_result | grep success)
		if [ -n "$unpair_success" ];then
			$($CLOUDAGENT_LOGGER '[XMPP Client][INFO] start cloudagent: unpair restful success')
			uci set cloudagent.cloudagent.do_unpair=0
			uci commit cloudagent
			break
		else
			$($CLOUDAGENT_LOGGER '[XMPP Client][INFO] start cloudagent: unpair restful failed, retrying')
			sleep $sleep_seconds
			sleep_seconds=$(($sleep_seconds*2))
			if [ $sleep_seconds -gt 600 ];then
				sleep_seconds=600
			fi
		fi
	done
fi

# generate a random number from 8~60
random_number=$(head /dev/urandom | hexdump -d | head -n 1 | cut -c 11-15)
sleep_seconds=$(expr $random_number % 53 + 8)
while true; do
	if [ -z "$restful_server_cert" ];then
		register_result=$($PKGPATH/bin/zyxel_device_register -s $restful_server_url -c '1005')
	else
		register_result=$($PKGPATH/bin/zyxel_device_register -s $restful_server_url -c $restful_server_cert)
	fi
	register_success=$(echo $register_result | grep success)

if [ -n "$register_success" ];then
	account=$(echo "$register_result" | awk -F':' '/xmpp_account/{print $2}')
	password=$(echo "$register_result" | awk -F':' '/xmpp_password/{print $2}')
	bot=$(echo "$register_result" | awk -F':' '/xmpp_bots/{printf("%s;",$2)}')
	ipaddr=$(echo "$register_result" | awk -F':' '/xmpp_ip_addresses/{printf("%s:%s;",$2,$3)}')
	stun_servers=$(echo "$register_result" | awk -F':' '/stun_ip_addresses/{printf("%s;",$2)}')
	owner_cloudid=$(echo "$register_result" | awk -F':' '/cloud_id/{printf("%s",$2)}')

	echo $owner_cloudid > /tmp/owner_cloud_user

	rm -rf /tmp/.cloudagent/agent_fifo

	$($CLOUDAGENT_LOGGER '[XMPP Client][INFO] start cloudagent: restful success, start cloud agent')
	#v 2.0 delete set_upnp_to_router
	#$PKGPATH/bin/set_upnp_to_router.sh &
        lock $LOCKFILE
        killall -9 zyxel_xmpp_client
	$PKGPATH/bin/zyxel_xmpp_client "$account" "$password" $bot $ipaddr > /dev/null 2>&1 &
        lock -u $LOCKFILE

	#clean white chain xmpp_server_list
	iptables -F xmpp_server_list
	iptables -Z xmpp_server_list
	#add xmpp server to white list
	serv_count=$(echo "$ipaddr" | grep -o ";" | wc -l)
	i=1
	while [ $i -le  "$serv_count" ]; do
		num_serv=$(echo "$ipaddr" | awk -F';' '{print $1}')
		ip=$(echo "$num_serv" | awk -F':' '{print $1}')
		port=$(echo "$num_serv" | awk -F':' '{print $2}')
		iptables -I xmpp_server_list -d $ip -p tcp --dport $port -j ACCEPT
		num_serv="$num_serv;"
		ipaddr=$(echo "$ipaddr" | sed "s/$num_serv//g")
		i=$(($i + 1))
	done
	restful_domain=$(echo $server_url | awk -F'/' '{print $3}')
	iptables -I xmpp_server_list -d $restful_domain -p tcp --dport 443 -j ACCEPT
	break
else
	$($CLOUDAGENT_LOGGER '[XMPP Client][INFO] start cloudagent: restful failed, retrying')
	sleep $sleep_seconds
	sleep_seconds=$(($sleep_seconds*2))
	if [ $sleep_seconds -gt 600 ];then
		sleep_seconds=600
	fi
fi
done
