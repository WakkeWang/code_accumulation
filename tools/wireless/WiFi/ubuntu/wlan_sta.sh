#!/bin/sh -e


if [ $# != 1 ] && [ $# != 2 ];then
	echo "usage: $0 up/down"
	echo "usage: $0 up wlan_name"
	exit 0
fi

wlan_name=wlp1s0
[ ! -z "$2" ] && wlan_name=$2

echo "Local wlan interface              :[$wlan_name]"

if [ "$1" = "down" ];then
	[ "`pidof wpa_supplicant`" != "" ] && killall wpa_supplicant

	ifconfig $wlan_name down && sleep 2 && ifconfig $wlan_name up
	ip addr flush dev $wlan_name
fi


if [ "$1" = "up" ];then
	[ "`pidof wpa_supplicant`" != "" ] && killall wpa_supplicant
	sleep 2
	wpa_supplicant -B -Dnl80211 -c /etc/wpa_supplicant/wpa_supplicant.conf -i$wlan_name
	sleep 1

	udhcpc -i $wlan_name -t 4 -n
	for i in $(seq 5); do

		if ifconfig $wlan_name | grep -q "inet addr:" ;then
			break;
		fi
		udhcpc -i $wlan_name -t 4 -n
	done

fi

exit 0
