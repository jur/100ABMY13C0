#!/bin/sh
Check_flag=/tmp/zy1905_wifi_reload
Loop_time=0
CHECK=0

RELOAD_WIFI_24G=/tmp/zy1905/reload_24G
RELOAD_WIFI_5G=/tmp/zy1905/reload_5G
RELOAD_WIFI_G1_24G=/tmp/zy1905/reload_G1_24G
RELOAD_WIFI_G1_5G=/tmp/zy1905/reload_G1_5G

#--------------------------------------------------------------------
#function
reload_Main24G() {
	echo "[zy1905] ---------------------  reload 24G Main"
	echo wifi0 > /tmp/WirelessDev
	echo ath0 > /tmp/wifi24G_Apply
	/etc/init.d/wireless restart
}

reload_Main5G() {
	echo "[zy1905] ---------------------  reload 5G Main"
	echo wifi1 > /tmp/WirelessDev
	echo ath10 > /tmp/wifi5G_Apply
	/etc/init.d/wireless restart
}

reload_Guest24G() {
	echo "[zy1905] ---------------------  reload 24G Guest"
	echo wifi0 > /tmp/WirelessDev
	echo ath1 > /tmp/wifi24G_Apply
	/etc/init.d/wireless restart
}

reload_Guest5G() {
	echo "[zy1905] ---------------------  reload 5G Guest"
	echo wifi1 > /tmp/WirelessDev
	echo ath11 > /tmp/wifi5G_Apply
	/etc/init.d/wireless restart
}
#--------------------------------------------------------------------


#if Check_flag exit, do nothing & exit
if [ -f $Check_flag ]; then
	echo "[zy1905] Has been run. Exit."
	exit
else
	echo "1" > $Check_flag
	echo "[zy1905] Do wifi reload."
fi

#sleep to wait
echo "[zy1905] sleep 8"
sleep 8

echo "[zy1905] wifi reload [start]"
uci commit wireless

#--------------------------------------------
while [ "$CHECK" -eq "0" ]
do
	echo "[zy1905] to check reload band"

	WIFI_24G=0
	WIFI_5G=0
	WIFI_G1_24G=0
	WIFI_G1_5G=0

	if [ -f $RELOAD_WIFI_24G ]; then
		WIFI_24G=1
	fi
	if [ -f $RELOAD_WIFI_5G ]; then
		WIFI_5G=1
	fi
	if [ -f $RELOAD_WIFI_G1_24G ]; then
		WIFI_G1_24G=1
	fi
	if [ -f $RELOAD_WIFI_G1_5G ]; then
		WIFI_G1_5G=1
	fi

	if [ "$WIFI_24G" -eq "0" ] && [ "$WIFI_5G" -eq "0" ] && [ "$WIFI_G1_24G" -eq "0" ] && [ "$WIFI_G1_5G" -eq "0" ]; then
		echo "[zy1905] ------------------------------------------  all band is reload"
		CHECK=1
	fi

	if [ "$WIFI_24G" -eq "1" ] && [ "$WIFI_5G" -eq "1" ] && [ "$WIFI_G1_24G" -eq "1" ] && [ "$WIFI_G1_5G" -eq "1" ]; then
		echo "[zy1905] ------------------------------------------  reload all"
		WIFI_24G=0
		WIFI_5G=0
		WIFI_G1_24G=0
		WIFI_G1_5G=0
		rm -rf $RELOAD_WIFI_24G
		rm -rf $RELOAD_WIFI_5G
		rm -rf $RELOAD_WIFI_G1_24G
		rm -rf $RELOAD_WIFI_G1_5G
		reload_Main24G
		reload_Main5G
		reload_Guest24G
		reload_Guest5G
	fi
	if [ "$WIFI_24G" -eq "1" ] && [ "$WIFI_G1_24G" -eq "1" ]; then
		echo "[zy1905] ------------------------------------------  reload 24G all"
		WIFI_24G=0
		WIFI_G1_24G=0
		rm -rf $RELOAD_WIFI_24G
		rm -rf $RELOAD_WIFI_G1_24G
		reload_Main24G
		reload_Guest24G
	fi
	if [ "$WIFI_5G" -eq "1" ] && [ "$WIFI_G1_5G" -eq "1" ]; then
		echo "[zy1905] ------------------------------------------  reload 5G all"
		WIFI_5G=0
		WIFI_G1_5G=0
		rm -rf $RELOAD_WIFI_5G
		rm -rf $RELOAD_WIFI_G1_5G
		reload_Main5G
		reload_Guest5G
	fi
	if [ "$WIFI_24G" -eq "1" ]; then
		echo "[zy1905] ------------------------------------------  reload 24G Main"
		WIFI_24G=0
		rm -rf $RELOAD_WIFI_24G
		reload_Main24G
	fi
	if [ "$WIFI_G1_24G" -eq "1" ]; then
		echo "[zy1905] ------------------------------------------  reload 24G Guest"
		WIFI_G1_24G=0
		rm -rf $RELOAD_WIFI_G1_24G
		reload_Guest24G
	fi
	if [ "$WIFI_5G" -eq "1" ]; then
		echo "[zy1905] ------------------------------------------  reload 5G Main"
		WIFI_5G=0
		rm -rf $RELOAD_WIFI_5G
		reload_Main5G
	fi
	if [ "$WIFI_G1_5G" -eq "1" ]; then
		echo "[zy1905] ------------------------------------------  reload 5G Guest"
		WIFI_G1_5G=0
		rm -rf $RELOAD_WIFI_G1_5G
		reload_Guest5G
	fi

	sleep 1
done
#--------------------------------------------

echo "[zy1905] wifi reload [End]"
rm -rf $Check_flag
sync

#check all is ready
if [ -f $RELOAD_WIFI_24G ]; then
	WIFI_24G=1
fi
if [ -f $RELOAD_WIFI_5G ]; then
	WIFI_5G=1
fi
if [ -f $RELOAD_WIFI_G1_24G ]; then
	WIFI_G1_24G=1
fi
if [ -f $RELOAD_WIFI_G1_5G ]; then
	WIFI_G1_5G=1
fi

if [ "$WIFI_24G" -eq "1" ] || [ "$WIFI_5G" -eq "1" ] || [ "$WIFI_G1_24G" -eq "1" ] || [ "$WIFI_G1_5G" -eq "1" ]; then
	sh /etc/zy1905/zy1905_wifi_reload.sh &
fi

exit
