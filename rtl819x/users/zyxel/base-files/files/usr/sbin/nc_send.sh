#!/bin/sh

if [ $# = 0 ]; then
    echo "usage: $0 filename ipaddr [port]"
    exit 1
fi

FILE=$1
IP=$2
PORT=${3:-60428}

nc $IP $PORT < $FILE
