#!/bin/sh

lock_file="/tmp/reload_zapi_service.lock"
zapi_service_pid=$(ps | grep zapi-service | grep -v grep | awk -F' ' '{printf $1}')

reload_zapi_service() {
    # echo "111 $zapi_service_pid"
    if [ ! -f "lock_file" ]; then
        touch $lock_file
    fi

    lock $lock_file

    if [ $1 == "force" ]; then
        echo "zapi-service need to force reload. Kill zapi-service !!!"
        killall zapi-service
        /bin/zapi-service &
    elif [ $1 == "normal" ]; then
        if [ -z "$zapi_service_pid" ]; then
            echo "Can not find zapi-service. start zapi-service."
            /bin/zapi-service &
        else
            echo "zapi-service is normal reload. Do nothing."
        fi
    else
        echo "invaild option"
    fi

    lock -u $lock_file
    # zapi_service_pid=$(ps | grep zapi-service | grep -v grep | awk -F' ' '{printf $1}')
    # echo "222 $zapi_service_pid"
}

print_help() {
    echo "./reload-zapi-service.sh [OPTION]"
    echo "    -n    Normal reload. If zapi-service daemon is alive will do nothing."
    echo "    -f    Force reload. If zapi-service daemon is alive will force to kill original zapi-service daemon and reload it."
}

case $1 in
    -n)
        # normal
        reload_zapi_service "normal"
    ;;
    -f)
        # force
        reload_zapi_service "force"
    ;;
    -h)
        print_help
    ;;
    *)
        # default normal
        reload_zapi_service "normal"
    ;;
esac