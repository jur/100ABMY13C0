#!/bin/sh
#. /lib/functions.sh

target_mac=$1
band_1=$2
band_2=$3
band_3=$4
band_4=$5

value_1=
value_2=
value_3=
value_4=

addband()
{
	local band="$1"

	if [ "$band" == "Main_24G" ] ; then
		value_1="0x00"
	fi

	if [ "$band" == "Main_5G" ] ; then
		value_2="0x01"
	fi

	if [ "$band" == "Guest1_24G" ] ; then
		value_3="0x20"
	fi

	if [ "$band" == "Guest1_5G" ] ; then
		value_4="0x21"
	fi
}


if [ "$target_mac" == "NULL" ] ; then
	target_mac="00:00:00:00:00:00"
fi

if [ "$band_1" != "Main_24G" ] && [ "$band_1" != "Main_5G" ] && [ "$band_1" != "Guest1_24G" ] && [ "$band_1" != "Guest1_5G" ] ; then
	echo "Usage:./wifiStateSync.sh [FreqBand]"
	echo "FreqBand :"
	echo "  Main_24G      Main_5G"
	echo "  Guest1_24G    Guest1_5G"
	exit
fi

if [ "$band_2" != "" ] && [ "$band_2" != "Main_24G" ] && [ "$band_2" != "Main_5G" ] && [ "$band_2" != "Guest1_24G" ] && [ "$band_2" != "Guest1_5G" ] ; then
	echo "Usage:./wifiStateSync.sh [FreqBand]"
	echo "FreqBand :"
	echo "  Main_24G      Main_5G"
	echo "  Guest1_24G    Guest1_5G"
fi

if [ "$band_3" != "" ] && [ "$band_3" != "Main_24G" ] && [ "$band_3" != "Main_5G" ] && [ "$band_3" != "Guest1_24G" ] && [ "$band_3" != "Guest1_5G" ] ; then
	echo "Usage:./wifiStateSync.sh [FreqBand]"
	echo "FreqBand :"
	echo "  Main_24G      Main_5G"
	echo "  Guest1_24G    Guest1_5G"
fi

if [ "$band_4" != "" ] && [ "$band_4" != "Main_24G" ] && [ "$band_4" != "Main_5G" ] && [ "$band_4" != "Guest1_24G" ] && [ "$band_4" != "Guest1_5G" ] ; then
	echo "Usage:./wifiStateSync.sh [FreqBand]"
	echo "FreqBand :"
	echo "  Main_24G      Main_5G"
	echo "  Guest1_24G    Guest1_5G"
fi

	addband $band_1
	addband $band_2
	addband $band_3
	addband $band_4

value=$value_1" "$value_2" "$value_3" "$value_4	

echo "target_mac = "$target_mac
#echo "value_1 = "$value_1
#echo "value_2 = "$value_2
#echo "value_3 = "$value_3
#echo "value_4 = "$value_4
echo "value=" $value

#DeviceMode=`uci get system.main.system_mode`
OpRole=`uci get system.main.operation_role`
#DeviceType
#Registrar renew
#if [ "$DeviceMode" = "1" ] ; then
if [ "$OpRole" = "controller" ] ; then
	echo "wifi sync renew"
	/usr/sbin/zy1905App 24 $target_mac $value
fi

