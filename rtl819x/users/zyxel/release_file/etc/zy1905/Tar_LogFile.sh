#!/bin/sh

TIMEOUT=30
CHECK=0
CHECK_START_FILE=/tmp/zy1905/zy1905_download_log_start_flag
CHECK_FINISH_FILE=/tmp/zy1905/zy1905_download_log_finish_flag
#FILE_1=/tmp/log/backhaul_flow_log
#FILE_2=/tmp/log/backhaul_flow_log.1
#FILE_3=/tmp/log/backhaul_log
#FILE_4=/tmp/log/backhaul_log.1
#FILE_5=/tmp/log/led.log
#FILE_6=/tmp/log/led.log.1

echo "tar log file... ... "
#echo "FILE_1"=$FILE_1
#echo "FILE_2"=$FILE_2
#echo "FILE_3"=$FILE_3
#echo "FILE_4"=$FILE_4

DIR=/tmp/zy1905/log
DIR_MANE=`date +%Y%m%d-%H-%M-%S`
DIR_PATH=$DIR/$DIR_MANE
FILE_NAME=$DIR_MANE.tar.gzip

#echo "DIR_PATH="$DIR_PATH

##### create dir
rm -rf $DIR
rm -rf $CHECK_START_FILE
rm -rf $CHECK_FINISH_FILE
mkdir -p $DIR_PATH

#check file exist
#if [ -f $FILE_1 ]; then
#	cp -a $FILE_1 $DIR_PATH
#fi
#if [ -f $FILE_2 ]; then
#	cp -a $FILE_2 $DIR_PATH
#fi
#if [ -f $FILE_3 ]; then
#	cp -a $FILE_3 $DIR_PATH
#fi
#if [ -f $FILE_4 ]; then
#	cp -a $FILE_4 $DIR_PATH
#fi
#if [ -f $FILE_5 ]; then
#	cp -a $FILE_5 $DIR_PATH
#fi
#if [ -f $FILE_6 ]; then
#	cp -a $FILE_6 $DIR_PATH
#fi

### diagnostic
FILE_DIAGNOSTIC=$DIR_PATH/diagnostic_log
diagnostic > $FILE_DIAGNOSTIC

#####  tar dif
#echo "tar dir($DIR_PATH)"
cd $DIR
tar -zc -f $FILE_NAME $DIR_MANE

#init vsftpd
sh /etc/zy1905/vsftpd_init "On" "$DIR" &
sleep 1

echo "Start to request download......."
#echo "$FILE_NAME"
echo "$FILE_NAME" > $CHECK_START_FILE

sync

while [ $CHECK -lt $TIMEOUT ]
do
	#echo "CHECK="$CHECK
	CHECK=$(($CHECK + 1))

	if [ -f $CHECK_FINISH_FILE ]; then
		#echo "$CHECK_FINISH_FILE exist......."
		rm -rf $CHECK_FINISH_FILE
		break;
	fi

	sleep 1
done

echo "Stop to request download......."
sh /etc/zy1905/vsftpd_init "Off" "NULL" &

rm -rf $DIR
rm -rf $CHECK_START_FILE
rm -rf $CHECK_FINISH_FILE

sync

exit

