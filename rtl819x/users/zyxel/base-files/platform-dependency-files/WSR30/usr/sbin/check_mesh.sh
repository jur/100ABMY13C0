#!/bin/sh

time_interval="60"

while true
do
	echo
	echo "############################################################"
	date
	echo "####################"
	cat /tmp/batman_debug/batman_adv/bat0/neighbors
	echo "####################"
	ifconfig
	echo "############################################################"
	sleep "$time_interval"
done

