#!/bin/sh
#. /lib/functions.sh
. /lib/config/uci.sh
 
#append() {
#        local var="$1"
#        local value="$2"
#        local sep="${3:- }"
# 
#        eval "export ${NO_EXPORT:+-n} -- \"$var=\${$var:+\${$var}\${value:+\$sep}}\$value\""
#}

band=$1
mkdir -p /tmp/run/
FILE_OUTPUT=/tmp/run/hostapd-zy1905_$band.conf

rm -f $FILE_OUTPUT

#echo "FILE_OUTPUT="$FILE_OUTPUT

if [ "$band" != "Main_24G" ] && [ "$band" != "Main_5G" ] && [ "$band" != "Guest1_24G" ] && [ "$band" != "Guest1_5G" ] && [ "$band" != "All" ]; then
	echo "Usage:./genhostapd.sh [FreqBand]"
	echo "FreqBand :"
	echo "  All"
	echo "  Main_24G      Main_5G"
	echo "  Guest1_24G    Guest1_5G"
else

if [ "$band" == "All" ] ; then
	echo "[zy1905] Main_24G"
	sh /etc/zy1905/genhostapd.sh Main_24G
	echo "[zy1905] Main_5G"
	sh /etc/zy1905/genhostapd.sh Main_5G
	echo "[zy1905] Guest1_24G"
	sh /etc/zy1905/genhostapd.sh Guest1_24G
	echo "[zy1905] Guest1_5G"
	sh /etc/zy1905/genhostapd.sh Guest1_5G
	echo "[zy1905] exit......."
exit
fi


if [ "$band" == "Main_24G" ] ; then
	vif="ath0"
fi
if [ "$band" == "Guest1_24G" ] ; then
	vif="ath1"
fi
if [ "$band" == "Main_5G" ] ; then
	vif="ath10"
fi
if [ "$band" == "Guest1_5G" ] ; then
	vif="ath11"
fi

#echo "*****PRINT DEBUG: VIF="$vif

	hostapd_cfg=

	enc=$(uci_get wireless $vif encryption)

	# use crypto/auth settings for building the hostapd config
	case "$enc" in
		NONE)
			wpa=0
		;;
		wpapsk|WPAPSK)
			wpa=1
			
			WPAPSKkey=$(uci_get wireless "$vif" WPAPSKkey)
			if [ "$WPAPSKkey" = "" ]; then
				WPAPSKkey=$(cat /tmp/tmppsk)
			fi
			#append hostapd_cfg "wpa_passphrase=$WPAPSKkey" "$N"
			echo "wpa_passphrase=$WPAPSKkey" >> $FILE_OUTPUT
			#append hostapd_cfg "wpa_pairwise=CCMP" "$N"
			echo "wpa_pairwise=CCMP" >> $FILE_OUTPUT
			#append hostapd_cfg "wpa_key_mgmt=WPA-PSK" "$N"
			echo "wpa_key_mgmt=WPA-PSK" >> $FILE_OUTPUT
		;;
		wpa2psk|WPA2PSK)
			wpapskcompatible=$(uci_get wireless "$vif" WPAPSKCompatible)
			if [ "$wpapskcompatible" = "0" ]; then
				wpa=2
			else
				wpa=3
			fi
			
			WPAPSKkey=$(uci_get wireless "$vif" WPAPSKkey)
			if [ "$WPAPSKkey" = "" ]; then
				WPAPSKkey=$(cat /tmp/tmppsk)
			fi
	
			#append hostapd_cfg "wpa_passphrase=$WPAPSKkey" "$N"
			echo "wpa_passphrase=$WPAPSKkey" >> $FILE_OUTPUT
			#append hostapd_cfg "wpa_pairwise=CCMP" "$N"
			echo "wpa_pairwise=CCMP" >> $FILE_OUTPUT
			#append hostapd_cfg "wpa_key_mgmt=WPA-PSK" "$N"
			echo "wpa_key_mgmt=WPA-PSK" >> $FILE_OUTPUT
		;;
		####################### //zy1905 not suppose mode
		wpa|WPA)
			wpa=1
			#append hostapd_cfg "wpa_key_mgmt=WPA" "$N"
			echo "wpa_key_mgmt=WPA" >> $FILE_OUTPUT
		;;
		wpa2|WPA2)
			wpa=2
			#append hostapd_cfg "wpa_key_mgmt=WPA2" "$N"
			echo "wpa_key_mgmt=WPA2" >> $FILE_OUTPUT
		;;
		#######################//zy1905 not suppose mode
		*)
			wpa=0
		;;
	esac

#echo "*****PRINT DEBUG: enc="$enc
#echo "*****PRINT DEBUG: wpa="$wpa
#echo "*****PRINT DEBUG: WPAPSKkey="$WPAPSKkey
#echo "*****PRINT DEBUG: wpapskcompatible="$wpapskcompatible

	#append hostapd_cfg "wpa=$wpa" "$N"
	echo "wpa=$wpa" >> $FILE_OUTPUT

##############WPS CONFIGURES START FROM HERE###############

	wps_enable=0
	[ "$vif" = "ath0" ] &&{
		wps_configured=$(uci_get wps wps conf)
	} 
	[ "$vif" = "ath1" ] &&{
		wps_configured=$(uci_get wps wps conf)
	}
	[ "$vif" = "ath10" ] &&{
		wps_configured=$(uci_get wps5G wps conf)
	}
	[ "$vif" = "ath11" ] &&{
		wps_configured=$(uci_get wps5G wps conf)
	}
#echo "*****PRINT DEBUG: wps_enable="$wps_enable
#echo "*****PRINT DEBUG: wps_configured="$wps_configured
	
	if [ "$wps_configured" = "0" ];then
		wps_state=1
	elif [ "$wps_configured" = "1" ];then
		wps_state=2
	fi
#echo "*****PRINT DEBUG: wps_state= "$wps_state

	#append hostapd_cfg "wps_state=$wps_state" "$N"
	echo "wps_state=$wps_state" >> $FILE_OUTPUT

	device_type=$(uci_get wireless "$vif" wps_device_type)
	if [ "$device_type" = "" ]; then
		device_type="6-0050F204-1"
	fi
		
	#append hostapd_cfg "device_type=$device_type" "$N"
	echo "device_type=$device_type" >> $FILE_OUTPUT
#echo "*****PRINT DEBUG: device_type= "$device_type

		[ "$vif" = "ath0" ] &&{
			#append hostapd_cfg "device_name=ZyXEL WSQ50 Main 2.4G" "$N"
			echo "device_name=ZyXEL WSQ50 Main 2.4G" >> $FILE_OUTPUT
		}
		[ "$vif" = "ath1" ] &&{
			#append hostapd_cfg "device_name=ZyXEL WSQ50 Guest1 2.4G" "$N"
			echo "device_name=ZyXEL WSQ50 Guest1 2.4G" >> $FILE_OUTPUT
		}
		[ "$vif" = "ath10" ] &&{
			#append hostapd_cfg "device_name=ZyXEL WSQ50 Main 5G" "$N"
			echo "device_name=ZyXEL WSQ50 Main 5G" >> $FILE_OUTPUT
		}
		[ "$vif" = "ath11" ] &&{
			#append hostapd_cfg "device_name=ZyXEL WSQ50 Guest1 5G" "$N"
			echo "device_name=ZyXEL WSQ50 Guest1 5G" >> $FILE_OUTPUT
		}

		#append hostapd_cfg "manufacturer=ZyXEL Communications Corp." "$N"
		echo "manufacturer=ZyXEL Communications Corp." >> $FILE_OUTPUT

		##append hostapd_cfg "wps_rf_bands=ag" "$N"
		
		[ "$vif" = "ath0" ] &&{
			rf_band=$(uci_get wireless wifi0 hwmode | sed 's/^11//')
		}
		[ "$vif" = "ath1" ] &&{
			rf_band=$(uci_get wireless wifi0 hwmode | sed 's/^11//')
		}
		[ "$vif" = "ath10" ] &&{
			rf_band=$(uci_get wireless wifi1 hwmode | sed 's/^11//')
		}
		[ "$vif" = "ath11" ] &&{
			rf_band=$(uci_get wireless wifi1 hwmode | sed 's/^11//')
		}
		#append hostapd_cfg "wps_rf_bands=$rf_band" "$N"
		echo "wps_rf_bands=$rf_band" >> $FILE_OUTPUT
#echo "*****PRINT DEBUG: rf_band="$rf_band
	
		#model_name=$(atsh | grep Product | awk '{print $NF}')
		#model_name=$(fw_printenv hostname | awk -F'=' '{print $2}' |sed 's/\"//g' | sed 's/://g')
		model_name=WSQ50
		#append hostapd_cfg "model_name=$model_name" "$N" 
		echo "model_name=$model_name" >> $FILE_OUTPUT
		#append hostapd_cfg "model_number=$model_name" "$N"
		echo "model_number=$model_name" >> $FILE_OUTPUT

		##append hostapd_cfg "serial_number=$(fw_printenv| grep serialnum| awk -F'=' '{print $(2)}')" "$N"
		#append hostapd_cfg "serial_number=S090Y00000000" "$N"
		echo "serial_number=S090Y00000000" >> $FILE_OUTPUT
		#append hostapd_cfg "config_methods=push_button display virtual_display virtual_push_button physical_push_button" "$N"
		echo "config_methods=push_button display virtual_display virtual_push_button physical_push_button" >> $FILE_OUTPUT

		#mac=$(fw_printenv ethaddr | awk -F'=' '{print $2}' |sed 's/\"//g' | sed 's/://g')
		#lower_mac=$(echo $mac | tr "A-Z" "a-z")
		mac=000000000000

		##append hostapd_cfg "uuid=87654321-9abc-def0-1234-$lower_mac" "$N"
		#append hostapd_cfg "uuid=87654321-9abc-def0-1234-$mac" "$N"
		echo "uuid=87654321-9abc-def0-1234-$mac" >> $FILE_OUTPUT

#echo "*****PRINT DEBUG: mac="$mac
#echo "*****PRINT DEBUG: lower_mac="$lower_mac
#echo "*****PRINT DEBUG: uuid=uuid=87654321-9abc-def0-1234-"$lower_mac

		
		#####ssid
		ssid=$(uci_get wireless "$vif" ssid)
		if [ "$ssid" = "" ]; then
			mac_ssid=$(ifconfig br-lan |grep HWaddr | awk -F'HWaddr ' '{print $2}' | sed 's/\"//g' | sed 's/://g' | cut -c 7-12)
			#echo "*****PRINT DEBUG: mac_ssid="$mac_ssid
			ssid=ZyXEL$mac_ssid
		fi
		#append hostapd_cfg "ssid=$ssid" "$N"
		echo "ssid=$ssid" >> $FILE_OUTPUT
#echo "*****PRINT DEBUG: ssid="$ssid
#echo $hostapd_cfg | sed 's/[ ]\([^ ]*=\)/\n\1/g' >> $FILE_OUTPUT
#echo "[zy1905] end...................."

	#get DeviceMode
	#DeviceMode=`uci get system.main.system_mode`
	#if [ "$DeviceMode" = "1" ] ; then
	OpRole=`uci get system.main.operation_role`
	if [ "$OpRole" = "controller" ] ; then
		if [ "$band" = "Main_24G" ]; then
			echo "1" > /tmp/zy1905/APautoconfig_24G
			/usr/sbin/zy1905App 6 0x00 &
		fi
		if [ "$band" = "Main_5G" ]; then
			echo "1" > /tmp/zy1905/APautoconfig_5G
			/usr/sbin/zy1905App 6 0x01 &
		fi
		if [ "$band" = "Guest1_24G" ]; then
			echo "1" > /tmp/zy1905/APautoconfig_G1_24G
			/usr/sbin/zy1905App 6 0x20 &
		fi
		if [ "$band" = "Guest1_5G" ]; then
			echo "1" > /tmp/zy1905/APautoconfig_G1_5G
			/usr/sbin/zy1905App 6 0x21 &
		fi
	fi
fi
