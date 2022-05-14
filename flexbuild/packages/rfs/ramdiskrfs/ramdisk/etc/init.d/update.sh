#!/bin/sh 

OK1012=ok1012-c
OK1043=ok1043-c
OK1046=ok1046-c
FILENAME=/mnt/config.ini
SECTION=config 

if [ -b /dev/sda1 ]
then
	mount /dev/sda1 /mnt
else
	echo "The first partition of the U disk was not found."
	exit
fi

if [ ! -e $FILENAME ]
then
	echo "$FILENAME not found."
	exit
fi

sed -e 's//\n/g' $FILENAME > ./config.ini
FILENAME=./config.ini

KEY=platform
platform=`awk -F '=' '/\['$SECTION'\]/{a=1}a==1&&$1~/'$KEY'/{print $2;exit}' $FILENAME`
KEY=sdfirmware
sdfirmware=`awk -F '=' '/\['$SECTION'\]/{a=1}a==1&&$1~/'$KEY'/{print $2;exit}' $FILENAME`
KEY=qspifirmware
qspifirmware=`awk -F '=' '/\['$SECTION'\]/{a=1}a==1&&$1~/'$KEY'/{print $2;exit}' $FILENAME`
KEY=rootfs
rootfs=`awk -F '=' '/\['$SECTION'\]/{a=1}a==1&&$1~/'$KEY'/{print $2;exit}' $FILENAME`
KEY=qspiflash
qspiflash=`awk -F '=' '/\['$SECTION'\]/{a=1}a==1&&$1~/'$KEY'/{print $2;exit}' $FILENAME`
KEY=emmcflash
emmcflash=`awk -F '=' '/\['$SECTION'\]/{a=1}a==1&&$1~/'$KEY'/{print $2;exit}' $FILENAME`

echo "=================================================================="
cat << EOM
+---------------+------------------------------------------------+
|platform	|$platform						
|rootfs		|$rootfs						
|qspiflash	|$qspiflash						
|emmcflash	|$emmcflash						
|sdfirmware	|$sdfirmware						
|qspifirmware	|$qspifirmware						
+---------------+------------------------------------------------+
EOM

if [ $platform == $OK1012 ];then
	devpartname=mmcblk1
else
	devpartname=mmcblk0
fi

qspi_flash()
{
	echo "=================================================================="
	echo "[erase qspi]"
	echo "=================================================================="
	flash_eraseall /dev/mtd0
	echo "=================================================================="
	echo "[flash qspi]"
	echo "=================================================================="
	echo "flashing, wait ..."
	time dd if=/mnt/$qspifirmware of=/dev/mtdblock0
	echo "flash done."
}

mmc_part_ls1012()
{
	echo "parting, wait ..."
        parted -s /dev/$devpartname mklabel msdos
        parted -s /dev/$devpartname unit s mkpart primary fat32 2048 206847
        parted -s /dev/$devpartname unit s mkpart primary ext4 206848 100%
	sync
	echo "part, done."
	echo "formating, wait ..."
	mkfs.vfat -n boot /dev/${devpartname}p1
	echo "format done."
}

mmc_part_ls1043_mmcboot()
{
	echo "parting, wait ..."
        parted -s /dev/$devpartname mklabel msdos
        parted -s /dev/$devpartname unit s mkpart primary fat32 2048 43007
        parted -s /dev/$devpartname unit s mkpart primary fat32 43008 247807
        parted -s /dev/$devpartname unit s mkpart primary ext4 247808 100%
	sync
	echo "part, done."
	echo "formating, wait ..."
	mkfs.vfat -n EFI /dev/${devpartname}p1
	mkfs.vfat -n boot /dev/${devpartname}p2
	echo "format, done."
}

mmc_partition_format()
{
	echo "=================================================================="
	echo "[emmc partition]"
	echo "=================================================================="

	dd if=/dev/zero of=/dev/$devpartname bs=512 count=2048
	
	if [ $platform == $OK1012 ]
	then
		mmc_part_ls1012
	fi

	if [ $platform == $OK1043 -o $platform == $OK1046 ]
	then
		mmc_part_ls1043_mmcboot
	fi
}

emmc_flash()
{
	echo "=================================================================="
	echo "[emmc flash]"
	echo "=================================================================="
	echo "flashing, wait..."
	mkdir /boot
	if [ $platform == $OK1012 ]
	then
		simg2img /mnt/$rootfs /dev/${devpartname}p2
		mount /dev/${devpartname}p1 /boot
		cp /mnt/boot -rf /boot/
		umount /boot
		sync
	fi

	if [ $platform == $OK1043 -o $platform == $OK1046 ]
	then
		dd if=/mnt/$sdfirmware of=/dev/$devpartname bs=512 seek=8
		simg2img /mnt/$rootfs /dev/${devpartname}p3
		mount /dev/${devpartname}p2 /boot
		cp /mnt/boot -rf /boot/
		umount /boot
		sync
	fi
	echo "flash, done."
}

starttime=`date +'%Y-%m-%d %H:%M:%S'`

if [ $qspiflash == "true" ]
then	
	qspi_flash
fi

if [ $emmcflash == "true" ]
then	
	mmc_partition_format
	emmc_flash
fi

umount /mnt
if [ $platform == $OK1012 ]
then
	mount /dev/${devpartname}p2 /mnt
fi

if [ $platform == $OK1043 -o $platform == $OK1046 ]
then
	mount /dev/${devpartname}p3 /mnt
fi

if [ -d /mnt/var/www ]
then
	chown www-data:www-data -R /mnt/var/www
fi

if [ -d /mnt/var/log/lighttpd ]
then
	chown www-data:www-data -R /mnt/var/log/lighttpd
fi

if [ -d /mnt/var/cache/lighttpd ]
then
	chown www-data:www-data -R /mnt/var/cache/lighttpd
fi

if [ -d /mnt/var/cache/man ]
then
	chown man:man -R /mnt/var/cache/man
fi

umount /mnt
sync
endtime=`date +'%Y-%m-%d %H:%M:%S'`
start_seconds=$(date --date="$starttime" +%s);
end_seconds=$(date --date="$endtime" +%s);
echo "[Done] "$((end_seconds-start_seconds))"s"
echo "=================================================================="
