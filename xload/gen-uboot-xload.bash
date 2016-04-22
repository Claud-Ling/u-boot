#!/bin/bash

#
# set image name and path
#
IMG_DIR=$(pwd)
IMG_NAME="u-boot.bin"
UIMG_NAME="uimage-uboot"
#CERT_ID=0001
#CERT_TYPE=8644_ES1_dev

# for debugging
#echo "IMG_DIR=$IMG_DIR"
#echo "IMG_NAME=$IMG_NAME"

function check_return()
{
    if [ $? -eq 0 ]; then
        echo "[ Success ]"
    else
        echo "[ Failed! ]"
        exit
    fi
}

function check_cmd()
{
    if [ $? -eq 0 ]; then
        echo "[ Success ]"
    else
        echo "[ Failed! ]"
        echo ""
        echo "    $1 is not found in the current path!"
        echo ""
        echo "    Please add the [MRUA_ROOT]/build_system/xbin into your path"
        echo "    i.e. export PATH=\$PATH:/[MRUA_ROOT]/build_system/xbin"
        echo ""
        exit
    fi
}

function check_env()
{
    if [ -z "$SMP8XXX_KEY_DOMAIN" ]; then
        echo ""
        echo " Waring: "
        echo " Please source [CPU package root]/CPU_KEYS_xload3.env"
        echo ""
        exit
    fi
}

#
# clean up before creating zbimage 
#
echo "==================================================="
echo " Preparing "
echo "==================================================="

check_env

echo -ne "    Cleanup directory .......... "
rm -f *.bin *.gz *.xload3 *.xload $UIMG_NAME $UIMG_NAME.pad
check_return

echo -ne "    Copy u-boot binary ......... "
cp ../$IMG_NAME ./ &>/dev/null
check_return

echo -ne "    Creating ./romfs ........... "
mkdir -p romfs &>/dev/null
check_return

#
# compress binary
#
echo "==================================================="
echo " Creating u-boot uimage "
echo "==================================================="

echo -ne "    Compressing u-boot image ... "
gzip -c9nf $IMG_DIR/$IMG_NAME > $IMG_NAME.gz
check_return

echo -ne "    Make uimage (ubzimage) ..... "
mkimage -A arm -O linux -T standalone -C none -a 0x81800000 -e 0x81800000 -C gzip -n u-boot -d $IMG_NAME.gz $UIMG_NAME >/dev/null
check_return 


echo "==================================================="
echo " Creating xload image"
echo "==================================================="

#
#cp -f $IMG_NAME.gz u-boot_gz.zbf
#bash build_cpu_xload.bash $IMG_DIR/$UIMG_NAME $SMP8XXX_CPU_CERTID $CERT_TYPE
create_xload.bash --payload $IMG_DIR/$UIMG_NAME --type cpu

echo "==================================================="
echo " Creating romfs"
echo "==================================================="

echo -ne "    Copying xload into ./romfs... "
cp -f  $UIMG_NAME.xload3 ./romfs/ &>/dev/null
check_return

echo -ne "    Generating romfs ............ "
genromfs -V UBOOT_XLOAD -d romfs -f  $UIMG_NAME-xload
check_return

echo -ne "    Rmove ./romfs ............... "
rm -rf romfs
check_return

echo "Xload image: " $UIMG_NAME-xload

echo "==================================================="
echo " Padding xloaded binary"
echo "==================================================="

cp $UIMG_NAME-xload $UIMG_NAME-xload.pad

# 2k page, we need 2 block 2*1024*64*2 blk
zeropad.bash $UIMG_NAME-xload.pad 262144

