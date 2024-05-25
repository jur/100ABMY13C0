#!/bin/sh

if [ ! -d /tmp/.cloudagent/resource_management ]; then
    mkdir -p /tmp/.cloudagent/resource_management
fi

if [ "${1}" = "beta" ]; then
    echo "init cloud resource management config for ${1} site"
    echo "https://rms-beta.zyxelonline.com" > /tmp/.cloudagent/resource_management/rms_api
    echo "WEdVayIaLj44ucivNDe1F6o99eWRxbt11rfp7hMV" > /tmp/.cloudagent/resource_management/x_api_key
elif [ "${1}" = "production" ]; then
    echo "init cloud resource management config for ${1} site"
    echo "https://rms.zyxelonline.com" > /tmp/.cloudagent/resource_management/rms_api
    echo "5lHmRVWaAm561pTlMwavI9z38cfe2K75aVqJWd3R" > /tmp/.cloudagent/resource_management/x_api_key
else
    echo "unknown argument"
fi