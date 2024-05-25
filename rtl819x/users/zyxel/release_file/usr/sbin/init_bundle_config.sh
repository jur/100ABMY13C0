init_bundle_config() {
    if [ "$#" -ne 1 ];then
        echo "Please check the input argument"
    fi
    if [ -e /usr/sbin/init_cloud_credentials.sh ]; then
        /usr/sbin/init_cloud_credentials.sh $1
    fi
    if [ -e /usr/sbin/init_cloud_resource_management.sh ]; then
        /usr/sbin/init_cloud_resource_management.sh $1
    fi
    if [ -e /usr/sbin/init_create_shorten_url_config.sh ]; then
        /usr/sbin//init_create_shorten_url_config.sh $1
    fi
    if [ -e /usr/sbin/init_push_notification_config.sh ]; then
        /usr/sbin//init_push_notification_config.sh $1
    fi
    if [ -e /usr/sbin/init_singlesignon.sh ]; then
        /usr/sbin//init_singlesignon.sh $1
    fi
}