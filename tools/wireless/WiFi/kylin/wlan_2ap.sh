#!/bin/sh -e

if [ $# != 1 ] && [ $# != 2 ];then
	echo "usage: $0 up/down"
	echo "usage: $0 up/down eth1/lte0"
	echo "		eth1/lte0: ap network gateway interface"
	exit 0
fi

wlan1_name=wlan0
wlan2_name=wlan1
[ ! -z "$2" ] && interface=$2

echo "Local wlan interface              :[$wlan1_name], [$wlan2_name]"
[ ! -z "$interface" ] && echo "AP External net through interface :[$interface]"

[ ! -z $wlan1_name ] && [ `ifconfig -a | grep $wlan1_name -c ` -ne 1 ] && echo "$wlan1_name is not found" && exit 1
[ ! -z $wlan2_name ] && [ `ifconfig -a | grep $wlan2_name -c ` -ne 1 ] && echo "$wlan2_name is not found" && exit 1
[ ! -z $interface ] && [ `ifconfig -a | grep $interface -c ` -ne 1 ] && echo "$interface is not found" && exit 1


if [ "$1" = "down" ];then
	systemctl stop dnsmasq.service
	[ "`pidof hostapd`" != "" ] && killall hostapd

	ifconfig $wlan1_name down && sleep 2 && ifconfig $wlan1_name up
	ifconfig $wlan2_name down && sleep 2 && ifconfig $wlan2_name up
	ip addr flush dev $wlan1_name
	ip addr flush dev $wlan2_name
fi


if [ "$1" = "up" ];then
	systemctl stop dnsmasq.service
	[ "`pidof hostapd`" != "" ] && killall hostapd
	sleep 2

	sed "/interface=/c\interface=$wlan1_name" -i /etc/hostapd/hostapd.conf
	sed "/interface=/c\interface=$wlan2_name" -i /etc/hostapd/hostapd1.conf

	hostapd -B /etc/hostapd/hostapd.conf
	hostapd -B /etc/hostapd/hostapd1.conf
	sleep 1

	ifconfig $wlan1_name 10.5.5.1/24
	ifconfig $wlan2_name 10.5.6.1/24
	systemctl restart dnsmasq.service

	if [ ! -z $interface ];then
		echo "Seting AP connect external network from $interface..."
		echo "1" > /proc/sys/net/ipv4/ip_forward
		iptables -t nat -A POSTROUTING -o $interface -j MASQUERADE
	fi
fi

exit 0
