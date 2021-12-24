#!/bin/bash

QL_DEVNAME=$1
QL_APN=$2
QL_USER=$3
QL_PASSWORD=$4
QL_NUMBER=$5

CONNECT="'chat -s -v ABORT BUSY ABORT \"NO CARRIER\" ABORT \"NO DIALTONE\" ABORT ERROR ABORT \"NO ANSWER\" TIMEOUT 30 \
\"\" AT OK ATE0 OK ATI\;+CSQ\;+CPIN?\;+COPS?\;+CGREG?\;\&D2 \
OK AT+CGDCONT=1,\\\"IP\\\",\\\"$QL_APN\\\",,0,0 OK ATD"$QL_NUMBER" CONNECT'"

pppd $QL_DEVNAME 115200 user "$QL_USER" password "$QL_PASSWORD" \
connect "'$CONNECT'" \
disconnect 'chat -s -v ABORT ERROR ABORT "NO DIALTONE" SAY "\nSending break to the modem\n" "" +++ "" +++ "" +++ SAY "\nGood \
noauth debug defaultroute noipdefault novj novjccomp noccp ipcp-accept-local ipcp-accept-remote ipcp-max-configure 30 local l&

