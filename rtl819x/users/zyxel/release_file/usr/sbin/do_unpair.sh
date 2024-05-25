#!/bin/sh

CLOUDAGENT_LOGGER="/usr/sbin/cloudagent_logger.sh"

restful_server_url=$(cat /tmp/.cloudagent/device_pair_api)
restful_server_cert=$(cat /tmp/.cloudagent/zyxel_cloud_agent_cert.conf)

echo "**** do cloud agent unpair *****"
echo "CloudAgent API: $restful_server_url"
echo "Cert Serial: $restful_server_cert"

unpair_result=$(/usr/bin/zyxel_pair_service -s $restful_server_url -r -c $restful_server_cert)
unpair_success=$(echo $unpair_resule | grep 'success')

if [ -n "$unpair_success" ]; then
    $($CLOUDAGENT_LOGGER 'do_unpair: unpair restful success')
else
    $($CLOUDAGENT_LOGGER 'do_unpair: unpair restful failed')
fi


