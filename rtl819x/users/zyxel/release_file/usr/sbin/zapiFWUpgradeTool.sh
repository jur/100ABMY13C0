#!/bin/sh
DL_INFO=/tmp/zyfw_info
RMS_INFO=/tmp/FWFile/fw_info
DLFILE=/tmp/fw.bin
PATH_INFO_FWFOLDER=/tmp/ApplicationData/FWFile
PATH_IMAGE_FWFOLDER=/tmp/wsr/FWFile

if [ -f "$DL_INFO" ]; then
        Zyinfo_size=$(cat /tmp/zyfw_info |grep Size|awk '{print$2}')
        Zyinfo_name=$(cat /tmp/zyfw_info |grep 'FW file'|awk '{print$3}')
        FTP_SITE=$(cat /config/ZYXEL_FTP_SITE)

        rm "$DLFILE"
        if [ "$Zyinfo_name" == "" ]; then
                echo "can not get FW file name"
                echo 5 > /tmp/FWupgrade_state
                return
        fi
        echo 0 > /tmp/FWupgrade_state
        /bin/wget -c "$FTP_SITE"/"$Zyinfo_name" -O "$DLFILE" &

        sleep 2
        actualsize=$(wc -c <"$DLFILE")
        while [ "$actualsize" != "$Zyinfo_size" ]; do
                chkwget=$(ps |grep wget | grep "grep" -v |awk '{print $1}')
                if [ -z "$chkwget" ]; then
                        rm "$DLFILE"
                        echo 5 > /tmp/FWupgrade_state
                        return
                fi
                sleep 2
                actualsize=$(wc -c <"$DLFILE")
        done
elif [ -f "$RMS_INFO" ]; then
        echo "hello" >/dev/console
        RMS_DL_SITE=/tmp/WSR30
        Zyinfo_name=$(cat "$RMS_INFO" |grep 'FWVersion' |awk -F '=' '{print$2}').bin
        Zyinfo_size=$(cat "$RMS_INFO" |grep 'FWSize' |awk -F '=' '{print$2}')
        rm $RMS_DL_SITE -rf
        /usr/sbin/FWupgradeTool -i 2 -f "$RMS_INFO" -d "/tmp" &
        sleep 2
        actualsize=$(wc -c < "$RMS_DL_SITE"/"$Zyinfo_name")
        #check download rate and size match info file
        while [ "$actualsize" != "$Zyinfo_size" ]; do
                chkcurl=$(ps |grep curl | grep "grep" -v |awk '{print $1}')
                if [ -z "$chkcurl" ]; then
                        rm $RMS_DL_SITE -rf
                        echo 5 > /tmp/FWupgrade_state
                        return
                fi
                sleep 2
                actualsize=$(wc -c < "$RMS_DL_SITE"/"$Zyinfo_name")
        done
        #check md5sum
        DL_CHECKSUM=$(cat "$RMS_DL_SITE"/"$Zyinfo_name".md5sum|awk {'print$1'})
        Zyinfo_checksum=$(cat "$RMS_INFO" |grep 'FWchecksum' |awk -F '=' '{print$2}')
        if [ $DL_CHECKSUM != $Zyinfo_checksum ]; then
                rm $RMS_DL_SITE -rf
                echo 5 > /tmp/FWupgrade_state
                return
        fi
        ln -s "$RMS_DL_SITE"/"$Zyinfo_name" $DLFILE
fi
kill -SIGUSR1 `cat /tmp/zyfwd.pid`

# LED behave as FWupgrade
/usr/bin/zyxel_led_ctrl FWupgrade

while [ true ]; do
        upgrade_state=$(cat /tmp/FWupgrade_state)
        if [ "$upgrade_state" == "2" ]; then
                rm -rf "$PATH_IMAGE_FWFOLDER"
                mkdir "$PATH_IMAGE_FWFOLDER"
                if [ -f "$RMS_INFO" ]; then
                        rm -rf "$PATH_INFO_FWFOLDER"
                        mkdir "$PATH_IMAGE_FWFOLDER"/WSR30
                        mkdir "$PATH_INFO_FWFOLDER"
                        cp -rp "$RMS_DL_SITE"/"$Zyinfo_name" "$PATH_IMAGE_FWFOLDER"/WSR30/"$Zyinfo_name"
                        cp -rp "$RMS_INFO" "$PATH_INFO_FWFOLDER"/
                else
                        cp -rp $DLFILE /tmp/wsr/FWFile/$Zyinfo_name
                fi
                /etc/zy1905/get_online_info_by_URL
                /usr/sbin/zy1905App 19 4 &
                #Preventing zy1905 upgrade counter for rebooting failed, add another counter for preventing this situation.
                sleep 240
                reboot -f
		return
        elif [ "$upgrade_state" == "5" ]; then
                rm "$DLFILE"
                rm $RMS_DL_SITE -rf
                return
        fi
	sleep 2
done

