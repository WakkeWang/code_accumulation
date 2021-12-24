```
ubuntu os:

dhcp service: systemctl restart isc-dhcp-server
	- /etc/dhcp/dhcpd.conf

kylin os:

dhcp service: systemctl restart dnsmasq.service
	- dnsmasq-2.76-7.ky3.kb2.aarch64.rpm
	- dnsmasq-utils-2.76-7.ky3.kb2.aarch64.rpm
	- /etc/dnsmasq.conf

network-manager
	nmcli 

```
