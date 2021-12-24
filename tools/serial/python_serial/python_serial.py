#!/usr/bin/env python3
import serial
import sys 

# sys.argv[0]   sys.argv[1]  sys.argv[2]
# pyserial.py   /dev/ttyAP0  /dev/ttyAP1

if len(sys.argv) != 3:
	print ("usage : ",sys.argv[0], "/dev/ttyAP0 /dev/ttyAP1")
	sys.exit(-1)
	
MSG=b"comtest"
	
baudrate_tup = ( 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 576000, 921600)
bytesize_tup = ( serial.SEVENBITS, serial.EIGHTBITS )
parity_tup = ( serial.PARITY_NONE, serial.PARITY_EVEN, serial.PARITY_ODD, serial.PARITY_MARK, serial.PARITY_SPACE )
stopbits_tup = ( serial.STOPBITS_ONE, serial.STOPBITS_ONE_POINT_FIVE, serial.STOPBITS_TWO )


def pair_test( port1, port2, baudrate1, bytesize1, parity1, stopbits1 ):
	ser1 = serial.Serial( port=port1, baudrate=baudrate1, bytesize=bytesize1, parity=parity1, stopbits=stopbits1)
	ser2 = serial.Serial( port=port2, baudrate=baudrate1, bytesize=bytesize1, parity=parity1, stopbits=stopbits1)
	
	w_count = ser1.write(MSG)

	idx = 0
	recv=b""
	while idx < w_count:
		recv_bytes = ser2.read(len(MSG))
		recv = recv + recv_bytes
		if len(recv_bytes) >= w_count:
			break
		idx = idx + len(recv_bytes)
	if MSG == recv:
		print (ser1.port,"to", ser2.port, baudrate1, bytesize1, stopbits1, parity1, "  OK")
	else:
		print (ser1.port, "to", ser2.port, baudrate1, bytesize1, stopbits1, parity1, "  Fail")
		
	ser1.close()
	ser2.close()



for baud in baudrate_tup: 
	for bytesz in bytesize_tup:
		for stopbit in stopbits_tup:
			for parity in parity_tup:
				pair_test(sys.argv[1], sys.argv[2], baud, bytesz, parity, stopbit)
				pair_test(sys.argv[2], sys.argv[1], baud, bytesz, parity, stopbit)


