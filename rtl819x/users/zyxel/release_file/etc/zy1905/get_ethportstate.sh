#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/lib
export PATH
export RTL8367RB_MDIO_IF=eth2

# . /etc/functions.sh
# include /lib/config

portnum=$1


if [ "$portnum" == "a" ]; then
   port=0
   while [ "$port" -le "2" ]
   do
        sh /etc/zy1905/get_ethportstate.sh $port
        port=$(($port+1))
   done
else
	##wan
	if [ "$portnum" == "0" ]; then 
		local up_5=$(cat /proc/portState | sed -e '1,5d' | grep 'up')
		if [ "$up_5" != "" ]; then
			local speed_5=$(cat /proc/portState | sed -e '1,5d' | grep 'up' | sed 's/^.*Speed://g'| sed 's/|UpTime.*$//g')
			if [ "$speed_5" == "10M" ]; then
				echo "Port $portnum: 10M" 
				return 0
			elif [ "$speed_5" == "100M" ]; then
				echo "Port $portnum: 100M" 
				return 0
			else
				echo "Port $portnum: 1000M" 
				return 0	
			fi
		else
		    echo "Port $portnum: off-link"
            exit 1
		fi
	fi
		##wan
	if [ "$portnum" == "1" ]; then 
		local up_4=$(cat /proc/portState | sed -e '1,2d' | sed -e '2,4d' | grep 'up')
		if [ "$up_4" != "" ]; then
			local speed_4=$(cat /proc/portState | sed -e '1,2d' | sed -e '2,4d' | grep 'up' | sed 's/^.*Speed://g'| sed 's/|UpTime.*$//g')
			if [ "$speed_4" == "10M" ]; then
				echo "Port $portnum: 10M" 
				return 0
			elif [ "$speed_4" == "100M" ]; then
				echo "Port $portnum: 100M" 
				return 0
			else
				echo "Port $portnum: 1000M" 
				return 0	
			fi
		else
		    echo "Port $portnum: off-link"
            exit 1
		fi
	fi   
fi
