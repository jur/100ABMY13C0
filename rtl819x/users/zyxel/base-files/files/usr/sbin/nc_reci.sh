#!/bin/sh

if [ $# = 0 ]; then
    echo "usage: $0 filename [port]"
    exit 1
fi


FILE=$1
PORT=${2:-60428}

nc -l -p $PORT > $FILE
