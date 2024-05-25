#!/bin/sh

if [ ! -d /tmp/.cloudagent/cloud_credentials ]; then
    mkdir -p /tmp/.cloudagent/cloud_credentials
fi

if [ "${1}" = "beta" ]; then
    echo "init cloud credentials config for ${1} site"
    echo "https://api-mycloud-beta.zyxel.com/d/3/credentials" > /tmp/.cloudagent/cloud_credentials/cloud_credentials_api
elif [ "${1}" = "production" ]; then
    echo "init cloud credentials config for ${1} site"
    echo "https://api-mycloud.zyxel.com/d/3/credentials" > /tmp/.cloudagent/cloud_credentials/cloud_credentials_api
else
    echo "unknown argument"
fi