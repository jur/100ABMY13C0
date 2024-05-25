#!/bin/sh

TIMEOUT=20
CHECK=0
CHECK_FINISH_FILE=/tmp/zy1905/zy1905_download_speedtest_finish_flag
RUNSPEEDTESTFTP_FLAG=/tmp/zy1905/zy1905_openftp_flag
DIR=$1

if [ -f $RUNSPEEDTESTFTP_FLAG ]; then
	echo "[zy1905 SpeedTest_openftp] $RUNSPEEDTESTFTP_FLAG is running."
	exit
fi
echo "1" > $RUNSPEEDTESTFTP_FLAG

echo "[zy1905 SpeedTest_openftp] SpeedTest opent ftp DIR="$DIR

##### create dir
rm -rf $CHECK_START_FILE
rm -rf $CHECK_FINISH_FILE

#init vsftpd
sh /etc/zy1905/vsftpd_init "On" "$DIR" &
sleep 1

echo "[zy1905 SpeedTest_openftp] Start to request download speedtest file......."

sync

while [ $CHECK -lt $TIMEOUT ]
do
	#echo "[zy1905] CHECK="$CHECK
	CHECK=$(($CHECK + 1))

	if [ -f $CHECK_FINISH_FILE ]; then
		echo "[zy1905 SpeedTest_openftp] $CHECK_FINISH_FILE exist......."
		break
	fi

	sleep 1
done

echo "[zy1905 SpeedTest_openftp] Stop to request download speedtest file......."
sh /etc/zy1905/vsftpd_init "Off" "NULL" &

rm -rf $CHECK_FINISH_FILE
rm -rf $RUNSPEEDTESTFTP_FLAG

sync

exit

