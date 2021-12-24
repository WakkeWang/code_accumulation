#!/bin/bash
if [ "$#" != "2" ] && [ "$#" != "3" ];then
   echo "usage of read :$0 2 25"
   echo "usage of write :$0 2 25 0"
   echo "usage of reset :$0 2 25 reset"
   exit -1
fi
base=$1
num=$2

HN=d2000

if [ "$HN" = "d2000" ]; then
	case $base in
		0)
			PHY_ADDRESS=0x28004000
			;;	    
		1)
			PHY_ADDRESS=0x28005000
			;;	    
		*)
			echo "base error"
			exit -1
	esac	
fi

if [ "$HN" = "d2000" ]; then
	DIR_REG=$((PHY_ADDRESS+0x4))
	DATA_REG=$((PHY_ADDRESS+0x0))
fi

DIR_VAL=`devmem2 $DIR_REG h | tail -1 | awk -F ": " '{print $2}'`
DATA_VAL=`devmem2 $DATA_REG h | tail -1 | awk -F ": " '{print $2}'`

bit_set=$((1<<num))
bit_clear=$((~(1<<num) & 0xFFFFFFFF))

#printf "bit_set = 0x%x\n" $bit_set
#printf "bit_clear = 0x%x\n" $bit_clear

GPIO_READ()
{
	if [ $(( DIR_VAL&(1<<num))) -eq $((1<<num)) ];then   
		if [ "$HN" = "d2000" ]; then
			DIR="out"
		else 
			DIR="in"
			DATA_VAL=`devmem2 $DATA_REG h`
		fi
	else
		if [ "$HN" = "d2000" ]; then
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
	if [ "$HN" = "d2000" ]; then
		DIR_VAL=$(( DIR_VAL | bit_set ))
	else 
		DIR_VAL=$(( DIR_VAL & bit_clear ))
	fi

	devmem2 $DIR_REG h $DIR_VAL > /dev/null

	DATA_VAL=$(( DATA_VAL | bit_set ))
	devmem2 $DATA_REG h $DATA_VAL > /dev/null

}

GPIO_Set_Low()
{
	# set to out_put
	if [ "$HN" = "d2000" ]; then
		DIR_VAL=$(( DIR_VAL | bit_set ))
	else 
		DIR_VAL=$(( DIR_VAL & bit_clear ))
	fi

	devmem2 $DIR_REG h $DIR_VAL > /dev/null

	DATA_VAL=$(( DATA_VAL & bit_clear ))
	devmem2 $DATA_REG h $DATA_VAL > /dev/null

}

#printf "=========Before===========\n"
#GPIO_READ

if [ "$#" = "2" ];then
	GPIO_READ
	exit 1
fi

if [ "$3" = "1" ];then
	GPIO_Set_High
elif [ "$3" = "0" ];then
    GPIO_Set_Low
elif [ "$3" = "reset" ];then
	GPIO_Set_High
	sleep 0.5
    GPIO_Set_Low
	sleep 0.5
    GPIO_Set_High
	sleep 0.5
fi

