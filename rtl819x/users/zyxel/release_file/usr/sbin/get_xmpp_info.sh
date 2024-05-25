#!/bin/sh

PKGPATH="/usr"

CLOUDAGENT_LOGGER="$PKGPATH/sbin/cloudagent_logger.sh"

#export CLOUDAGENT_INSTALL_DIR=$PKGPATH

mkdir -p /tmp/.cloudagent

changeSite=$(uci get cloudagent.cloudagent.site_flag)
if [ "$changeSite" == "beta" ]; then
	cp /usr/share/zyxel_beta_cloud_agent_cert.conf -a /tmp/.cloudagent/zyxel_cloud_agent_cert.conf
	server_url=$(cat /usr/share/zyxel_beta_cloud_agent_configure.conf | awk -F'=' '{print $2}')
else
	cp /usr/share/zyxel_cloud_agent_cert.conf -a /tmp/.cloudagent/zyxel_cloud_agent_cert.conf
	server_url=$(cat /usr/share/zyxel_cloud_agent_configure.conf | awk -F'=' '{print $2}')
fi

#echo $PKGPATH > /tmp/.cloudagent/agent_path

echo $server_url/user/1/token > /tmp/.cloudagent/validate_api
echo $server_url/d/3/register > /tmp/.cloudagent/device_register_api
echo $server_url/d/1/pairing > /tmp/.cloudagent/device_pair_api

restful_server_url=$(cat /tmp/.cloudagent/device_register_api)
restful_server_cert=$(cat /tmp/.cloudagent/zyxel_cloud_agent_cert.conf)

if [ -z "$restful_server_cert" ];then
	register_result=$($PKGPATH/bin/zyxel_device_register -s $restful_server_url -c '1005')
else
	register_result=$($PKGPATH/bin/zyxel_device_register -s $restful_server_url -c $restful_server_cert)
fi
success=$(echo $register_result | grep success)

if [ -n "$success" ];then
	account=$(echo "$register_result" | awk -F':' '/xmpp_account/{print $2}')
	password=$(echo "$register_result" | awk -F':' '/xmpp_password/{print $2}')
	bot=$(echo "$register_result" | awk -F':' '/xmpp_bots/{printf("%s;",$2)}')
	serv_ip=$(echo "$register_result" | awk -F':' '/xmpp_ip_addresses/{printf("%s:%s;",$2,$3)}')
	stun_servers=$(echo "$register_result" | awk -F':' '/stun_ip_addresses/{printf("%s;",$2)}')
	owner_cloudid=$(echo "$register_result" | awk -F':' '/cloud_id/{printf("%s",$2)}')
	echo $owner_cloudid > /tmp/owner_cloud_user
	$($CLOUDAGENT_LOGGER '[XMPP Client][INFO] agent reconnect: restful success')
	echo -e "$account""\n""$password""\n"$bot"\n"$serv_ip

	#clean white chain xmpp_server_list
	iptables -F xmpp_server_list
	iptables -Z xmpp_server_list
	#add xmpp server to white list
	serv_count=$(echo "$serv_ip" | grep -o ";" | wc -l)
	i=1
	while [ $i -le  "$serv_count" ]; do
		num_serv=$(echo "$serv_ip" | awk -F';' '{print $1}')
		ip=$(echo "$num_serv" | awk -F':' '{print $1}')
		port=$(echo "$num_serv" | awk -F':' '{print $2}')
		iptables -I xmpp_server_list -d $ip -p tcp --dport $port -j ACCEPT
		num_serv="$num_serv;"
		serv_ip=$(echo "$serv_ip" | sed "s/$num_serv//g")
		i=$(($i + 1))
	done
else
	echo "failed"
fi
