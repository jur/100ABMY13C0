#!/bin/sh

if [ ! -d /tmp/.cloudagent/push_notification ]; then
    mkdir -p /tmp/.cloudagent/push_notification/
fi

if [ "${1}" = "beta" ]; then
    echo "init push notification config for ${1} site"
    echo "https://api-mycloud-beta.zyxel.com/d/3/notifications" > /tmp/.cloudagent/push_notification/push_notify_api
    echo "33054861-8fbf-41ec-beb0-c4405df8e4e9" > /tmp/.cloudagent/push_notification/app_group_id
elif [ "${1}" = "production" ]; then
    echo "init push notification config for ${1} site"
    echo "https://api-mycloud.zyxel.com/d/3/notifications" > /tmp/.cloudagent/push_notification/push_notify_api
    echo "83d538c7-d265-4166-bfcc-bc077e685011" > /tmp/.cloudagent/push_notification/app_group_id
else
    echo "unknown argument"
fi