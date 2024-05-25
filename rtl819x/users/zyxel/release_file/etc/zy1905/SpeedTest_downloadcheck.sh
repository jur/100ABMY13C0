#!/bin/sh

TIMEOUT=10
CHECK=0
CHECKSPEEDTEST_FLAG=$1
SPEEDTEST_FILE=$2
SPEEDTEST_DOWNLOAD_FLAG=$SPEEDTEST_FILE.flag
Result=0

echo "[zy1905 SpeedTest_downloadcheck] CHECKSPEEDTEST_FLAG="$CHECKSPEEDTEST_FLAG
echo "[zy1905 SpeedTest_downloadcheck] SPEEDTEST_FILE="$SPEEDTEST_FILE

echo "[zy1905 SpeedTest_downloadcheck] check speedtest file......."
rm -f $CHECKSPEEDTEST_FLAG
sync

while [ $CHECK -lt $TIMEOUT ]
do
	#echo "[zy1905 SpeedTest_downloadcheck] CHECK="$CHECK
	CHECK=$(($CHECK + 1))

	if [ -f $SPEEDTEST_DOWNLOAD_FLAG ]; then
		echo "[zy1905 SpeedTest_downloadcheck] $SPEEDTEST_DOWNLOAD_FLAG exist"

		if [ ! -f $SPEEDTEST_FILE ]; then
			#init SpeedTest info
			echo "[zy1905 SpeedTest_downloadcheck] $SPEEDTEST_FILE not exist"
			Result=0
		else
			#init SpeedTest info
			echo "[zy1905 SpeedTest_downloadcheck] $SPEEDTEST_FILE exist"
			#echo -n 0,0,0 > /tmp/SpeedTestInfo
			Result=1
		fi
		break
	fi

	sleep 1
done

#Download finish
echo "[zy1905 SpeedTest_downloadcheck] Download speedtest file ($Result)......."

echo "$Result" > $CHECKSPEEDTEST_FLAG

sync

exit

