# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2022 NXP

# Initialize our own variables:
target_model="la1224"
log_level="info"
build_variant="release"
boot_mode="pci"
features=""

TOOLS_DIR=$PWD/../../tools
RELEASE_DIR=$PWD/release
RELEASE_TMP_DIR=$PWD/release_tmp

show_help()
{
       echo "Usage: ./$1 -t <target_model> -l <log_level> -b <build_variant> -m <boot_mode> -f <features>"
       echo "       target_model = {simu, cfp, la1246, la1224, la1224cpe, la1238rdb, la1238cpe}"
       echo "       log_level = {err, info, dbg, isr, all}"
       echo "       build_variant = {debug, release}"
       echo "       boot_mode = {pebm, pci}"
       echo "       features = {LA12XX_FEATURE_L1C_REFAPP=ON/OFF, LA12XX_FEATURE_RUDEMO=ON/OFF, LA12XX_FEATURE_WARMUP=ON/OFF,
		LA12XX_FEATURE_IDLECALIBRATE=ON/OFF, LA12XX_DRIVER_HOST_CLI=ON/OFF, LA12XX_DRIVER_PATRON_RF=ON/OFF,
		LA12XX_DRIVER_ICEWINGS_RF=ON/OFF, LA12XX_DRIVER_DIORA_RF_LIB=ON/OFF }"
}
while getopts "ht:l:b:m:c:f:" opt; do
       case "$opt" in
               h)  show_help
                   exit 0
                       ;;
               t)  target_model=$OPTARG
                       ;;
               l)  log_level=$OPTARG
                       ;;
               b)  build_variant=$OPTARG
                       ;;
               m)  boot_mode=$OPTARG
                       ;;
               f)  features+=":$OPTARG"
                       ;;
       esac
done
shift $((OPTIND -1))

echo "build.sh: target_model=$target_model, log_level=$log_level, build_variant=$build_variant boot_mode=$boot_mode features=$features"

echo "******************************Compiling x86 depedency tools***************************************************"
ELF_PARSER_PATH=$TOOLS_DIR/elf_parser
ELF_PARSER_BIN=elf_parse

cd $ELF_PARSER_PATH; rm -f $ELF_PARSER_BIN
gcc -o $ELF_PARSER_BIN elf_parse.c

if [ ! -f "$ELF_PARSER_BIN" ];
then
echo "elf_parser not found, Exiting...."
exit 0
fi
cd -

echo "$ELF_PARSER_PATH/$ELF_PARSER_BIN"

if [ "$boot_mode" == "xspi" ];
then
XSPI_HEADER_PATH=$TOOLS_DIR/xspi_boot_header
XSPI_HEADER_BIN=xspi_boot_header

cd $XSPI_HEADER_PATH; rm -f $XSPI_HEADER_BIN
gcc -o $XSPI_HEADER_BIN xspi_boot_header.c

if [ ! -f "$XSPI_HEADER_BIN" ];
then
echo "xspi_boot_header not found, Exiting...."
exit 0
fi
cd -

echo "$XSPI_HEADER_PATH/$XSPI_HEADER_BIN"
fi

echo "./clean.sh"
./clean.sh
mkdir -p $RELEASE_DIR $RELEASE_TMP_DIR

echo "******************************Compiling***************************************************"
echo "./core_build.sh $target_model $log_level $build_variant $boot_mode"
sh core_build.sh $target_model $log_level $build_variant $boot_mode $features 2>&1 > core_build.log

echo "**************************************Elf Parsing*********************************************"
cp $RELEASE_DIR/geul_e200.elf $RELEASE_TMP_DIR
$ELF_PARSER_PATH/$ELF_PARSER_BIN $RELEASE_DIR/geul_e200.elf
if [ "$boot_mode" == "xspi" ];
then
$XSPI_HEADER_PATH/$XSPI_HEADER_BIN $RELEASE_DIR/geul_e200.elf
mv geul_ibr.bin $RELEASE_DIR/geul_ibr.bin
fi
echo "*******Moving files to Release Directory******************"
echo "=>$RELEASE_DIR"


mv -v $RELEASE_TMP_DIR/geul_e200.bin.0 $RELEASE_DIR/ > /dev/null 2>&1
mv -v $RELEASE_TMP_DIR/geul_e200.bin.1 $RELEASE_DIR/ > /dev/null 2>&1
mv -v $RELEASE_TMP_DIR/geul_e200.bin.2 $RELEASE_DIR/ > /dev/null 2>&1
mv -v $RELEASE_TMP_DIR/geul_e200.bin.3 $RELEASE_DIR/ > /dev/null 2>&1
mv -v $RELEASE_TMP_DIR/geul_e200.bin.4 $RELEASE_DIR/ > /dev/null 2>&1
mv -v $RELEASE_TMP_DIR/geul_e200.bin.5 $RELEASE_DIR/ > /dev/null 2>&1
mv -v $RELEASE_TMP_DIR/geul_e200.bin.6 $RELEASE_DIR/ > /dev/null 2>&1
mv -v $RELEASE_TMP_DIR/geul_e200.bin.7 $RELEASE_DIR/ > /dev/null 2>&1

#mv -v $output_file $RELEASE_DIR

echo "Deleting temporary files"
cd ..
rm -rf $RELEASE_TMP_DIR
rm -f $ELF_PARSER_PATH/$ELF_PARSER_BIN
if [ "$boot_mode" == "xspi" ];
then
rm -f $XSPI_HEADER_PATH/$XSPI_HEADER_BIN
dd if=$RELEASE_DIR/geul_ibr.bin of=$RELEASE_DIR/tmp.bin > /dev/null 2>&1
dd if=/dev/zero bs=1M count=1 of=$RELEASE_DIR/zero.bin > /dev/null 2>&1
cat $RELEASE_DIR/tmp.bin $RELEASE_DIR/zero.bin > $RELEASE_DIR/padd.bin
truncate --size=1M $RELEASE_DIR/padd.bin
cat $RELEASE_DIR/padd.bin $RELEASE_DIR/geul_e200.elf > $RELEASE_DIR/geul_e200_xspi.bin
rm -f $RELEASE_DIR/tmp.bin
rm -f $RELEASE_DIR/zero.bin
rm -f $RELEASE_DIR/padd.bin
fi
