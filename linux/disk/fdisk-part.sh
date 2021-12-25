#!/bin/sh

#  2@1@16@mmcblk0@1,vfat,8G,format_true@2,ext3,4G,format_true
#  partition_num = 2
#  part_true = 1
#  skip_sector = 16
#  device = mmcblk0
#  partition 1: vfat,8G,format_true
#  Partition 2: ext3,4G,format_true

# Watch Here !!!
# when skip_sector != 16 (mmc device) or skip_sector != 63 (sda device)
# disk only can have one partition , otherwise will error,
# this is casued by fdisk, it cannot skip the free sectors reserved.

part_num="`echo $1 | awk -F '@' '{print $1}'`"
part_true="`echo $1 | awk -F '@' '{print $2}'`"
skip_sector="`echo $1 | awk -F '@' '{print $3}'`"
dev=`echo $1 | awk -F '@' '{print $4}'`

part1_flag=`echo $1 | awk -F '@' '{print $5}'`
part2_flag=`echo $1 | awk -F '@' '{print $6}'`
part3_flag=`echo $1 | awk -F '@' '{print $7}'`
part4_flag=`echo $1 | awk -F '@' '{print $8}'`

device=/dev/$dev
if [ ! -b $device ];then
	echo "[ ERROR]  $device not found, or not a block device"
	echo "[ ERROR]  Exit from $0"
	exit 1
fi

[ `echo $device | grep mmcblk[0-9] -c` -eq 1 ] && flag="p"
[ `echo $device | grep nvme -c` -eq 1 ] && flag="p"

killall udevd mdev syslogd klogd 2> /dev/null
echo "" > /proc/sys/kernel/hotplug 2> /dev/null

cd / && umount `cat /proc/mounts | grep $dev | awk '{print $2}'` 2> /dev/null

if [ ! -z $part1_flag ];then

	part1_operate=n # add a new partition
	part1_type=p    # primary partition
	part1_index=`echo $part1_flag | awk -F ',' '{print $1}'`
	part1_fstype=`echo $part1_flag | awk -F ',' '{print $2}'`
	part1_size=`echo $part1_flag | awk -F ',' '{print $3}'`
	part1_format="`echo $part1_flag | awk -F ',' '{print $4}'`"

	if [ "$part1_index" = "$part_num" ] && [ "$part1_size" = "none" ];then
		part1_size=""
	else
		part1_size="+`echo $part1_flag | awk -F ',' '{print $3}'`"
	fi

	MKFS_1=`which mkfs.$part1_fstype`
	if [ "$MKFS_1" = "" ];then
		[ `which mke2fs` != "" ] && MKFS_1="`which mke2fs` -F -t $part1_fstype"
		[ `which mke2fs` = "" ] && echo "Not found mkfs command, exit" && exit 1
	elif [ "$part1_fstype" != "vfat" ];then
		MKFS_1="`which mkfs.$part1_fstype` -F"
	fi

	if [ "$part1_fstype" = "vfat" ];then
		part1_toggle=t
		part1_toggle_num=$part1_index
		part1_toggle_code=c
	fi
fi

if [ ! -z $part2_flag ];then
	part2_operate=n # add a new partition
	part2_type=p    # primary partition
	part2_index=`echo $part2_flag | awk -F ',' '{print $1}'`
	part2_fstype=`echo $part2_flag | awk -F ',' '{print $2}'`
	part2_size=`echo $part2_flag | awk -F ',' '{print $3}'`
	part2_format="`echo $part2_flag | awk -F ',' '{print $4}'`"

	if [ "$part2_index" = "$part_num" ] && [ "$part2_size" = "none" ];then
		part2_size=""
	else
		part2_size="+`echo $part2_flag | awk -F ',' '{print $3}'`"
	fi

	MKFS_2=`which mkfs.$part2_fstype`
	if [ "$MKFS_2" = "" ];then
		[ `which mke2fs` != "" ] && MKFS_2="`which mke2fs` -F -t $part2_fstype"
		[ `which mke2fs` = "" ] && echo "Not found mkfs command, exit" && exit 1
	elif [ "$part2_fstype" != "vfat" ];then
		MKFS_2="`which mkfs.$part2_fstype` -F"
	fi

	if [ "$part2_fstype" = "vfat" ];then
		part2_toggle=t
		part2_toggle_num=$part2_index
		part2_toggle_code=c
	fi
fi

if [ ! -z $part3_flag ];then
	part3_operate=n # add a new partition
	part3_type=p    # primary partition
	part3_index=`echo $part3_flag | awk -F ',' '{print $1}'`
	part3_fstype=`echo $part3_flag | awk -F ',' '{print $2}'`
	part3_size=`echo $part3_flag | awk -F ',' '{print $3}'`
	part3_format="`echo $part2_flag | awk -F ',' '{print $4}'`"

	if [ "$part3_index" = "$part_num" ] && [ "$part3_size" = "none" ];then
		part3_size=""
	else
		part3_size="+`echo $part3_flag | awk -F ',' '{print $3}'`"
	fi
	MKFS_3=`which mkfs.$part3_fstype`
	if [ "$MKFS_3" = "" ];then
		[ `which mke2fs` != "" ] && MKFS_3="`which mke2fs` -F -t $part3_fstype"
		[ `which mke2fs` = "" ] && echo "Not found mkfs command, exit" && exit 1
	elif [ "$part3_fstype" != "vfat" ];then
		MKFS_3="`which mkfs.$part3_fstype` -F"
	fi

	if [ "$part3_fstype" = "vfat" ];then
		part3_toggle=t
		part3_toggle_num=$part3_index
		part3_toggle_code=c
	fi
fi

if [ ! -z $part4_flag ];then
	part4_operate=n # add a new partition
	part4_type=p    # extend partition
	part4_index=`echo $part4_flag | awk -F ',' '{print $1}'`
	part4_fstype=`echo $part4_flag | awk -F ',' '{print $2}'`
	part4_size=`echo $part4_flag | awk -F ',' '{print $3}'`
	part4_format="`echo $part4_flag | awk -F ',' '{print $4}'`"

	if [ "$part4_index" = "$part_num" ] && [ "$part4_size" = "none" ];then
		part4_size=""
	else
		part4_size="+`echo $part4_flag | awk -F ',' '{print $3}'`"
	fi
	MKFS_4=`which mkfs.$part4_fstype`
	if [ "$MKFS_4" = "" ];then
		[ `which mke2fs` != "" ] && MKFS_4="`which mke2fs` -F -t $part4_fstype"
		[ `which mke2fs` = "" ] && echo "Not found mkfs command, exit" && exit 1
	elif [ "$part4_fstype" != "vfat" ];then
		MKFS_4="`which mkfs.$part4_fstype` -F"
	fi

	if [ "$part4_fstype" = "vfat" ];then
		part4_toggle=t
		part4_toggle_num=$part4_index
		part4_toggle_code=c
	fi
fi


if [ $part_true -eq 1 ];then
	dd if=/dev/zero of=$device bs=1M count=5 conv=fsync 2> /dev/null
	busybox fdisk -u $device > /dev/null << EOF
${part1_operate}
${part1_type}
${part1_index}
${skip_sector}
${part1_size}

${part2_operate}
${part2_type}
${part2_index}

${part2_size}

${part3_operate}
${part3_type}
${part3_index}

${part3_size}

${part4_operate}
${part4_type}

${part4_size}

${part1_toggle}
${part1_toggle_num}
${part1_toggle_code}

${part2_toggle}
${part2_toggle_num}
${part2_toggle_code}

${part3_toggle}
${part3_toggle_num}
${part3_toggle_code}

${part4_toggle}
${part4_toggle_num}
${part4_toggle_code}

w
EOF
fi

busybox partprobe
sleep 2

[ ! -z $part1_fstype ] && [ "$part1_format" = "format_true" ] \
		&& $MKFS_1 ${device}${flag}${part1_index} > /dev/null
[ ! -z $part2_fstype ] && [ "$part2_format" = "format_true" ]\
		&& $MKFS_2 ${device}${flag}${part2_index} > /dev/null
[ ! -z $part3_fstype ] && [ "$part3_format" = "format_true" ] \
		&& $MKFS_3 ${device}${flag}${part3_index} > /dev/null
[ ! -z $part4_fstype ] && [ "$part3_format" = "format_true" ]\
		&& $MKFS_4 ${device}${flag}${part4_index} > /dev/null

exit 0
