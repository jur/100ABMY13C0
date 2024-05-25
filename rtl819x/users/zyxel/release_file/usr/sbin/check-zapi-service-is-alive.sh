#!/bin/sh

lock_file="/tmp/check_zapi_service_is_alive.lock"
if [ ! -f "lock_file" ]; then
        touch $lock_file
fi

pid_lock=$(ps | grep $lock_file | grep -v grep | awk -F ' ' '{print $1}')

if [ -n "$pid_lock" ]; then
    echo "have exist lock, exit !!!" > /dev/console
    exit
fi

lock $lock_file

zapi-cli -s 8

lock -u $lock_file

