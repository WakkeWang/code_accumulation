#!/bin/bash


IFNAME="wlan"
STATUS=$1
SSID=$2
PASSWORD=$3

NUM=`ps -aux |grep " /usr/sbin/NetworkManager" |grep -v grep`
if [ "$NUM"x == ""x ];then
        exit 0
fi

if [ "$STATUS"x == "up"x ]; then
        NUM=`nmcli c |grep $IFNAME |awk '{print $1}'| wc -l`
        for((i=1;i<=$NUM;i++));
        do
                LINE_NUM=`nmcli c |grep $IFNAME |awk -v var=$i 'NR==var {print NF}'`
                if [ "$LINE_NUM"x == "0"x ] || [ "$LINE_NUM"x == ""x ] ; then
                        break;
                elif [ "$LINE_NUM" -ge 4 ]; then
                        UUID=`nmcli c |grep $IFNAME |awk -v var=$i 'NR==var {print $(NF-2)}'`
                        nmcli connection delete "$UUID"
                        ((i--))
                fi

        done

        if [ "$SSID"x == ""x ];then
                SSID=`cat /etc/wpa_supplicant.conf |grep ssid|awk -F "[\"\"]" 'NR==1 {print $2}'`
                PASSWORD=`cat /etc/wpa_supplicant.conf |grep ps|awk -F "[\"\"]" 'NR==1 {print $2}'`
        fi
        if [ "$SSID"x != ""x ];then
                nmcli device wifi connect "$SSID" password "$PASSWORD" ifname wlan0 name wlan0
        fi


elif [ "$STATUS"x == "down"x ]; then
        NUM=`nmcli c |grep $IFNAME |awk '{print $1}'| wc -l`
        for((i=1;i<=$NUM;i++));
        do
                LINE_NUM=`nmcli c |grep $IFNAME |awk -v var=$i 'NR==var {print NF}'`
                if [ "$LINE_NUM"x == "0"x ] || [ "$LINE_NUM"x == ""x ] ; then
                        break;
                elif [ "$LINE_NUM" -ge 4 ]; then
                        UUID=`nmcli c |grep $IFNAME |awk -v var=$i 'NR==var {print $(NF-2)}'`
                        nmcli connection delete "$UUID"
                        ((i--))
                fi

        done
fi


