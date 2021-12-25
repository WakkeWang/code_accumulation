## test tools

# Features supported:

	- read or write ethernet phy registers

	- serial port RS232, RS485 test

	- read or write eeprom

```
root@localhost:~# ./tools 
help:

LogLevel:
  -l                                               log level from 0-5,default is 3
Mii:
  --miiread=<interface,register_addr>              read phy register
  --miiwrite=<interface,register_addr,value>       write phy register
Serial:
  --looptest=<device1,device2,baud>                loop test at 8n1
  --speedtest=<device1,device2,highest_baud>       speed from 1200 -> highest_baud -> 921600
eeprom:
  --burneeff=<i2c_dev,dev_addr,off,file>           burn eeprom from file
  --readeetf=<i2c_dev,dev_addr,off,len>            read eeprom to file: ./eeprom
  --readee=<i2c_dev,dev_addr,type,off>             hexdump eeprom value at off
                                                   type=[b,w,l], which represents 1/2/4 bytes
  --writee=<i2c_dev,dev_addr,type,off,value>       write value to eeprom at off
                                                   type=[b,w,l], which represents 1/2/4 bytes

```



