#!/bin/sh

DOWNLOAD_URL=$1
SAVE_DIR=$2
FILE_NAME=$3
CHECK_FINISH_FILE=$SAVE_DIR/$FILE_NAME.flag

# echo "download file..."
# echo "DOWNLOAD_URL"=$DOWNLOAD_URL
# echo "SAVE_DIR"=$SAVE_DIR
# echo "FILE_NAME"=$FILE_NAME

mkdir -p $SAVE_DIR
rm -rf $CHECK_FINISH_FILE

/etc/zy1905/Output_log.sh "[zy1905 Download] download URL = $DOWNLOAD_URL" &

wget $DOWNLOAD_URL -O $SAVE_DIR/$FILE_NAME -T 20 || rm -f $SAVE_DIR/$FILE_NAME

sync

if [ ! -f $SAVE_DIR/$FILE_NAME ]; then
	echo "[zy1905 Download] fail > $CHECK_FINISH_FILE"
	echo "fail" > $CHECK_FINISH_FILE
	/etc/zy1905/Output_log.sh "[zy1905 Download] download fail"
else
	echo "[zy1905 Download] success > $CHECK_FINISH_FILE"
	echo "success" > $CHECK_FINISH_FILE
	/etc/zy1905/Output_log.sh "[zy1905 Download] download success"
fi


exit

