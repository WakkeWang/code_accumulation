if [ "$#" != "1" ];then
        echo "usage: $0 [1/2]"
        echo "parameters and options:"
        echo "[1/2] :which SIM slot "
        exit 
fi


gpio3_15()
{
	if [ -d /sys/class/gpio/gpio111 ];then
	
		echo 0 > /sys/class/gpio/gpio111/value
		sleep 1
		echo 1 > /sys/class/gpio/gpio111/value
		sleep 8
	
	else
		echo 111 > /sys/class/gpio/export
		echo "out" > /sys/class/gpio/gpio111/direction

		echo 0 > /sys/class/gpio/gpio111/value
		sleep 1
		echo 1 > /sys/class/gpio/gpio111/value
		sleep 8
	fi
}

killall pppd
                                         
if [ -d /sys/class/gpio/gpio4 ];then
        if [ "$1" == "1" ];then
		echo "choose SIM slot :1"
      		echo 1 > /sys/class/gpio/gpio4/value
        elif [ "$1" == "2" ];then   
	        echo "choose  SIM slot :2"
        	echo 0 > /sys/class/gpio/gpio4/value
        fi                                                                
else                                   
        echo 4 > /sys/class/gpio/export
		echo "out" > /sys/class/gpio/gpio4/direction
        if [ "$1" == "1" ];then        
		echo "choose SIM slot :1"
      		echo 1 > /sys/class/gpio/gpio4/value
        elif [ "$1" == "2" ];then                   
	        echo "choose  SIM slot :2"
        	echo 0 > /sys/class/gpio/gpio4/value
        fi                                          
fi      
       
gpio3_15


