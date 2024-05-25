#!/bin/sh
. /sbin/functions.sh
# . /lib/functions/boot.sh
# include /lib/upgrade

RESULT_FILE=/tmp/FWupgrade_state
CHECK=0
RESULT_OUTPUT=/tmp/Flag_FWtest_result

echo "[zy1905 fw_check] Run FW upgrade fw_check... ... "

FILE=$1

rm -f $RESULT_FILE
rm -f $RESULT_OUTPUT

if [ ! -f $FILE ]; then
	echo "[zy1905 fw_check] $FILE not exist"
	echo "0" > $RESULT_OUTPUT
	exit
fi

######## FW upgrade check
echo "[zy1905 fw_check] Start to run fw_upgrade check & wait 3 sec"
# fw_upgrade fw_check $FILE 1> $RESULT_FILE 2> /dev/null
kill -SIGUSR2 `cat /tmp/zyfwd.pid`
sleep 3

if [ -f $RESULT_FILE ]; then
	echo "[zy1905 fw_check] search file=$RESULT_FILE"
	while [ "$CHECK" -eq "0" ]
	do
		Result=`cat $RESULT_FILE`
		echo "[zy1905 fw_check] Success Result = $Result"
		if [ "$Result" == "1" ]; then
			CHECK=1
			echo "1" > $RESULT_OUTPUT
			/etc/zy1905/Output_log.sh "[zy1905 fw_check] Upgrade check success"

			exit
		elif [ "$Result" == "1" ]; then
			echo "[zy1905 fw_check] Wait FWupgrade"
		fi

		# Result=`cat $RESULT_FILE | grep "Wrong firmware image header"`
		# echo "Wrong Result = $Result"
		# if [ "$Result" != "" ]; then
		# 	echo "0" > $RESULT_OUTPUT
		# 	exit
		# else
		# 	echo "Not find"
		# fi

		sleep 1
	done
fi

exit
