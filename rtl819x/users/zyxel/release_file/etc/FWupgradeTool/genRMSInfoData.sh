#!/bin/sh

fw_version=$(/sbin/atsh | grep "LD   Version" | awk '{print $4}')

mac_address=$(/sbin/atsh | grep First | awk '{print $5}')
mac_address=$(echo $mac_address | tr '[:lower:]' '[:upper:]')

serial_number=$(/sbin/atsh | grep Serial | awk '{print $4}')

model=$(/sbin/atsh | grep "Product Model" | awk '{print $4}')

echo "RMSinfo = fw_version=$fw_version&mac_address=$mac_address&serial_number=$serial_number"
echo "fw_version=$fw_version&mac_address=$mac_address&serial_number=$serial_number" > /tmp/RMSinfo.conf
echo "$model" >> /tmp/RMSinfo.conf

exit


