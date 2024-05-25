#!/bin/sh

# echo "action: $1"
# echo "session_id: $2"
# echo "from: $3"

action=$1
session_id=$2
from=$3
api_v=$4

pid=$$

if [ "$action" == "app_send_data_model" ];then
	if [ "$#" != "5" ];then
		echo "-1"
	else
		#echo $pid
		file_name=$5

		/usr/sbin/cloudagent_logger.sh "[XMPP Client][DEBUG] ($session_id) => $(cat $file_name)"

		zapi-cli -i "$file_name" -o "/tmp/.cloudagent/$pid.txt"

		if [ -e "$file_name" ];then
			rm $file_name
		fi

		if [ -e "/tmp/.cloudagent/$pid.txt" ];then
			response_data_model=$(cat /tmp/.cloudagent/$pid.txt)
			/usr/sbin/cloudagent_logger.sh "[XMPP Client][DEBUG] ($session_id) <= $response_data_model"
			echo app_send_data_model $session_id $from $api_v success /tmp/.cloudagent/$pid.txt > /tmp/.cloudagent/agent_fifo
		else
			/usr/sbin/cloudagent_logger.sh "[XMPP Client][DEBUG] exec_cmd.sh: zapi-cli execute failed!"
			echo app_send_data_model $session_id $from $api_v failed 494 > /tmp/.cloudagent/agent_fifo
		fi

	fi
	exit 0
fi
