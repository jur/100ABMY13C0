#!/bin/sh

LOG_FILE="/tmp/.cloudagent/logs/cloudagent.log"
LOG_FILE_ALT="/tmp/.cloudagent/logs/cloudagent.log.1"

LOG_FILE_L="/tmp/.cloudagent/logs/cloudagent_l.log"
LOG_FILE_L_ALT="/tmp/.cloudagent/logs/cloudagent_l.log.1"

LOG_FILE_ALT_BACKUP="/tmp/ApplicationData/cloudagent/cloudagent.log.1"
LOG_FILE_L_ALT_BACKUP="/tmp/ApplicationData/cloudagent/cloudagent_l.log.1"

if [ "$#" -ne 1 ]; then
	exit 1
fi
# check the bakup log path exists
if [ ! -d '/tmp/ApplicationData/cloudagent' ]; then
	mkdir -p '/tmp/ApplicationData/cloudagent'
fi

# check the log path exists
if [ ! -d '/tmp/.cloudagent/logs' ]; then
	mkdir -p '/tmp/.cloudagent/logs'
fi

timestamp=$(date +%Y/%m/%d-%H:%M:%S)

# string length
str=$1
strlen=${#str}
if [ $strlen -gt 512 ]; then
	str=$(echo $str | cut -c1-512)">>"
fi

# log file not exists
if [ ! -f $LOG_FILE ]; then
	echo "[$timestamp] $str" > $LOG_FILE
else
	# log file exists, count line of log file first
	line_count=$(wc -l $LOG_FILE | awk '{print $1}')

	if [ $line_count -gt 400 ]; then
		cp $LOG_FILE $LOG_FILE_ALT_BACKUP
		mv $LOG_FILE $LOG_FILE_ALT
		echo "[$timestamp] $str" > $LOG_FILE
	else
		echo "[$timestamp] $str" >> $LOG_FILE
	fi
fi

# longer log file
# log file not exists
if [ ! -f $LOG_FILE_L ]; then
	echo "[$timestamp] $1" > $LOG_FILE_L
else
	# log file exists, count line of log file first
	line_count=$(wc -l $LOG_FILE_L | awk '{print $1}')

	if [ $line_count -gt 2000 ]; then
		cp $LOG_FILE_L $LOG_FILE_L_ALT_BACKUP
		mv $LOG_FILE_L $LOG_FILE_L_ALT
		echo "[$timestamp] $1" > $LOG_FILE_L
	else
		echo "[$timestamp] $1" >> $LOG_FILE_L
	fi
fi
