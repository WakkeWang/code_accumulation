#!/bin/sh

if [ "$#" != "2" ] && [ "$#" != "3" ];then
   echo "usage of read :$0 2 25"
   echo "usage of write :$0 2 25 0"
   echo "usage of reset :$0 2 25 reset"
   exit -1
fi

base=$1
num=$2

HN=`cat /proc/board | tr A-Z a-z`

if [ "$HN" == "ecu1155" ]; then
	case $base in
		1)
			PHY_ADDRESS=0x0209C000
			;;
		2)
			PHY_ADDRESS=0x020A0000
			;;
		3)
			PHY_ADDRESS=0x020A4000
			;;
		4)
			PHY_ADDRESS=0x020A8000
			;;
		5)
			PHY_ADDRESS=0x020AC000
			;;
		6)
			PHY_ADDRESS=0x020B0000
			;;
		7)
			PHY_ADDRESS=0x020B4000
			;;
		*)
			echo "base error"
			exit -1
	esac
elif [ "$HN" == "ecu1253" ]; then
	PCLK_GPIO=$base
	case $base in
		0)
			PHY_ADDRESS=0xFF040000
			PCLK_GPIO=${base}_pmu
			;;	    
		1)
			PHY_ADDRESS=0xFF250000
			;;	    
		2)
			PHY_ADDRESS=0xFF260000
			;;	    
		3)
			PHY_ADDRESS=0xFF270000
			;;
		*)
			echo "base error"
			exit -1
	esac
else 
	case $base in
		0)
			PHY_ADDRESS=0x44E07000
			;;	    
		1)
			PHY_ADDRESS=0x4804C000
			;;	    
		2)
			PHY_ADDRESS=0x481AC000
			;;	    
		3)
			PHY_ADDRESS=0x481AE000
			;;
		*)
			echo "base error"
			exit -1
	esac
fi

if [ "$HN" == "ecu1155" ] || [ "$HN" == "ecu1253" ]; then
	DIR_REG=$[$PHY_ADDRESS+0x4]
	DATA_REG=$[$PHY_ADDRESS+0x0]
else
	DIR_REG=$[$PHY_ADDRESS+0x134]
	DATA_REG=$[$PHY_ADDRESS+0x13C]
fi

DIR_VAL=`devmem $DIR_REG`
DATA_VAL=`devmem $DATA_REG`

bit_set=$((1<<num))
bit_clear=$((~(1<<num) & 0xFFFFFFFF))

#printf "bit_set = 0x%x\n" $bit_set
#printf "bit_clear = 0x%x\n" $bit_clear

GPIO_READ()
{
	if [ $(( DIR_VAL&(1<<num))) -eq $((1<<num)) ];then   
		if [ "$HN" == "ecu1155" ] || [ "$HN" == "ecu1253" ]; then
			DIR="out"
		else 
			DIR="in"
			DATA_REG=$[$PHY_ADDRESS+0x138]
			DATA_VAL=`devmem $DATA_REG`
		fi
	else
		if [ "$HN" == "ecu1155" ] || [ "$HN" == "ecu1253" ]; then
			DIR="in"
		else 
			DIR="out"
		fi
	fi

	if [ $(( DATA_VAL&(1<<num))) -eq $((1<<num)) ];then   
		VAL=1
	else
		VAL=0
	fi
	printf "Get GPIO%d DIR_REG :0x%x[ 0x%x ] GPIO%d_%d = %s\n" $base $DIR_REG $DIR_VAL $base $num $DIR
	printf "Get GPIO%d DATA_REG :0x%x[ 0x%x ] GPIO%d_%d = %d\n" $base $DATA_REG $DATA_VAL $base $num $VAL
}

GPIO_Set_High()
{
	# set to out_put
	if [ "$HN" == "ecu1155" ] || [ "$HN" == "ecu1253" ]; then
		DIR_VAL=$(( $DIR_VAL | $bit_set ))
	else 
		DIR_VAL=$(( $DIR_VAL & $bit_clear ))
	fi
	if [ "$HN" == "ecu1253" ]; then       
		echo 1 > /sys/kernel/debug/clk/pclk_gpio$PCLK_GPIO/clk_enable_count
	fi 
	devmem $DIR_REG 32 $DIR_VAL

	DATA_VAL=$(( $DATA_VAL | $bit_set ))
	devmem $DATA_REG 32 $DATA_VAL
	if [ "$HN" == "ecu1253" ]; then       
		echo 0 > /sys/kernel/debug/clk/pclk_gpio$PCLK_GPIO/clk_enable_count
	fi 
}

GPIO_Set_Low()
{
	# set to out_put
	if [ "$HN" == "ecu1155" ] || [ "$HN" == "ecu1253" ]; then
		DIR_VAL=$(( $DIR_VAL | $bit_set ))
	else 
		DIR_VAL=$(( $DIR_VAL & $bit_clear ))
	fi
	if [ "$HN" == "ecu1253" ]; then       
		echo 1 > /sys/kernel/debug/clk/pclk_gpio$PCLK_GPIO/clk_enable_count
	fi 
	devmem $DIR_REG 32 $DIR_VAL

	DATA_VAL=$(( $DATA_VAL & $bit_clear ))
	devmem $DATA_REG 32 $DATA_VAL
	if [ "$HN" == "ecu1253" ]; then       
		echo 0 > /sys/kernel/debug/clk/pclk_gpio$PCLK_GPIO/clk_enable_count
	fi 
}

#printf "=========Before===========\n"
#GPIO_READ

if [ "$#" == "2" ];then
	GPIO_READ
	exit -1
fi

if [ "$3" == "1" ];then
	GPIO_Set_High
elif [ "$3" == "0" ];then
    GPIO_Set_Low
elif [ "$3" == "reset" ];then
	GPIO_Set_High
	sleep 0.5
    GPIO_Set_Low
	sleep 0.5
    GPIO_Set_High
	sleep 0.5
fi
