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
	INT_MASK_REG=$((PHY_ADDRESS+0x1c))
	INT_TYPE_REG=$((PHY_ADDRESS+0x20))
	INT_POLARITY_REG=$((PHY_ADDRESS+0x24))
	INT_EN_REG=$((PHY_ADDRESS+0x18))
fi

DIR_VAL=`devmem2 $DIR_REG h | tail -1 | awk -F ": " '{print $2}'`
INT_MASK_VAL=`devmem2 $GPIO_INT_MASK h | tail -1 | awk -F ": " '{print $2}'`
INT_TYPE_VAL=`devmem2 $GPIO_INT_TYPE h | tail -1 | awk -F ": " '{print $2}'`
INT_POLARITY_VAL=`devmem2 $GPIO_INT_POLARITY h | tail -1 | awk -F ": " '{print $2}'`
INT_EN_VAL=`devmem2 $GPIO_INT_EN h | tail -1 | awk -F ": " '{print $2}'`

set -x

bit_set=$((1<<num))
bit_clear=`echo $((1<<num)) | awk '{print compl($0)}'`
bit_clear=$(( bit_clear & 0xFF))

bit_clear=$((~(1<<num) & 0xFF))
printf "%#x\n,bit_clear"

DIR_VAL=$(( DIR_VAL & bit_clear ))
INT_MASK_VAL=$(( INT_MASK_VAL & bit_clear ))
INT_TYPE_VAL=$(( INT_TYPE_VAL & bit_set ))
INT_POLARITY_VAL=$(( INT_POLARITY_VAL & bit_clear ))
INT_EN_VAL=$(( INT_EN_VAL & bit_set ))

exit 0

devmem2 $DIR_REG h $DIR_VAL > /dev/null
devmem2 $INT_MASK_REG h $INT_MASK_VAL > /dev/null
devmem2 $INT_TYPE_REG h $INT_TYPE_VAL > /dev/null
devmem2 $INT_POLARITY_REG h $INT_POLARITY_VAL > /dev/null
devmem2 $INT_EN_REG h $INT_EN_VAL > /dev/null

