#!/bin/sh -e

if [ $# != 1 ] && [ $# != 2 ] && [ $# != 3 ];then
	echo "usage: $0 up/down"
	echo "usage: $0 up/down wlan0"
	echo "usage: $0 up/down wlan0 eth1/lte0"
	echo "		wlan0: local wlan interface"
	echo "		eth1/lte0: ap network gateway interface"
	exit 0
fi

#interface=eth1
wlan_name=wlp1s0
[ ! -z "$2" ] && wlan_name=$2
[ ! -z "$3" ] && interface=$3

echo "Local wlan interface              :[$wlan_name]"
[ ! -z "$interface" ] && echo "AP External net through interface :[$interface]"

[ ! -z $wlan_name ] && [ `ifconfig -a | grep $wlan_name -c ` -ne 1 ] && echo "$wlan_name is not found" && exit 1
[ ! -z $interface ] && [ `ifconfig -a | grep $interface -c ` -ne 1 ] && echo "$interface is not found" && exit 1


if [ "$1" = "down" ];then
	service isc-dhcp-server stop
	[ "`pidof hostapd`" != "" ] && killall hostapd
	[ "`pidof wpa_supplicant`" != "" ] && killall wpa_supplicant

	ifconfig $wlan_name down && sleep 2 && ifconfig $wlan_name up
	ip addr flush dev $wlan_name
fi


if [ "$1" = "up" ];then
	service isc-dhcp-server stop
	[ "`pidof hostapd`" != "" ] && killall hostapd
	[ "`pidof wpa_supplicant`" != "" ] && killall wpa_supplicant
	sleep 2

	sed "/INTERFACES=/c\INTERFACES=$wlan_name" -i /etc/default/isc-dhcp-server
	sed "/interface=/c\interface=$wlan_name" -i /etc/hostapd/hostapd.conf

	hostapd -B /etc/hostapd/hostapd.conf
	sleep 1

	ifconfig $wlan_name 10.5.5.1/24
	service isc-dhcp-server restart

	if [ ! -z $interface ];then
		echo "Seting AP connect external network from $interface..."
		echo "1" > /proc/sys/net/ipv4/ip_forward
		iptables -t nat -A POSTROUTING -o $interface -j MASQUERADE
	fi
fi

exit 0
