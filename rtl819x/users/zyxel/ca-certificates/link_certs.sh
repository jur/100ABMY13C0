#!/bin/sh

echo "$1"

echo "start link certs."

file_list=`ls -1 $1/etc/ssl/certs`

for file_name in $file_list
do
	HASH=`openssl x509 -subject_hash_old -fingerprint -noout -in $1/etc/ssl/certs/$file_name | tr "\n\r" " " | awk -F ' ' '{print $1}'`
	SUFFIX=0
	if [ -f "$1/etc/ssl/certs/$HASH.$SUFFIX" ] ; then
		let SUFFIX=$SUFFIX+1
	fi
	# ln -s "/etc/ssl/certs/$file_name" "$file_name"
	# ln -s "$file_name" "$1/etc/ssl/certs/$HASH.$SUFFIX"
	# echo "ln $1/etc/ssl/certs/$file_name $1/etc/ssl/certs/$HASH.$SUFFIX"
done

echo "link certs is done."