#!/bin/bash

OPERATOR=$1
TTYUSB=$2

echo "4G module ppp dialing"

QL_NAME=$1
QL_DEVNAME=$2
QL_APN=3gnet
QL_USER=user
QL_PASSWORD=123456
QL_NUMBER=*99#

if [ $QL_NAME=="unicom" ]; then
        QL_APN=3gnet
        QL_NUMBER=*99#
        /usr/bin/ppp_general.sh $QL_DEVNAME $QL_APN $QL_USER $QL_PASSWORD $QL_NUMBER
        sleep 5
        NUM=`nmcli c | grep ppp0 |grep -v grep`
        if [ "$NUM"x == "1"x ];then
                echo "operate name   $QL_NAME"
                exit 0
        fi
elif [ $QL_NAME=="telecom" ]; then
        QL_APN=""
        QL_NUMBER=#777
        /usr/bin/ppp_general.sh $QL_DEVNAME $QL_APN $QL_USER $QL_PASSWORD $QL_NUMBER
        sleep 5
        NUM=`nmcli c | grep ppp0 |grep -v grep`
        if [ "$NUM"x == "1"x ];then
                echo "operate name   $QL_NAME"
                exit 0
        fi
elif [ $QL_NAME=="cmnet" ]; then
        QL_APN=cmnet
        QL_NUMBER=*99*1#
        /usr/bin/ppp_general.sh $QL_DEVNAME $QL_APN $QL_USER $QL_PASSWORD $QL_NUMBER
        sleep 5
        NUM=`nmcli c | grep ppp0 |grep -v grep`
        if [ "$NUM"x == "1"x ];then
                echo "operate name   $QL_NAME"
                exit 0
        fi
elif [ $QL_NAME=="default" ]; then
        QL_APN=3gnet
        QL_NUMBER=*99#
        /usr/bin/ppp_general.sh $QL_DEVNAME $QL_APN $QL_USER $QL_PASSWORD $QL_NUMBER
        sleep 5
        NUM=`nmcli c | grep ppp0 |grep -v grep`
        if [ "$NUM"x == "1"x ];then
                echo "default operate name   $QL_NAME"
                exit 0
        elif [ "$NUM"x == "0"x ]; then
                QL_APN=""
                QL_NUMBER=#777
                /usr/bin/ppp_general.sh $QL_DEVNAME $QL_APN $QL_USER $QL_PASSWORD $QL_NUMBER
                sleep 5
                NUM=`nmcli c | grep ppp0 |grep -v grep`
                if [ "$NUM"x == "1"x ];then
                        echo "default operate name   $QL_NAME"
                        exit 0
                elif [ "$NUM"x == "0"x ]; then
                        QL_APN=cmnet
                        QL_NUMBER=*99*1#
                        /usr/bin/ppp_general.sh $QL_DEVNAME $QL_APN $QL_USER $QL_PASSWORD $QL_NUMBER
                        sleep 5
                        NUM=`nmcli c | grep ppp0 |grep -v grep`
                        if [ "$NUM"x == ""x ];then
                        echo "Invalid Operation"
                        exit 0
                        fi
                fi

        fi
fi

