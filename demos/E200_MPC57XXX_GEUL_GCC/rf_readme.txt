1. update the external submodules
git submodule update --init
- it will bring RF drivers from NXP internal repo

or you can define the RF_DRIVER e.g DIORA path externally as:
export RF_DRIVER_PATH=/home/nxa15309/Diora-SDK-baseline-v1.0.1

2. Build for DIORA RF (with pre-build library)
./clean.sh; ./build.sh -t la1224 -l info -b debug -m pci -f LA12XX_FEATURES=ON -f LA12XX_DRIVER_DIORA_RF_LIB=ON

3. Or Build for PATRON-YUCCA RF
./clean.sh; ./build.sh -t la1224 -l info -b debug -m pci -f LA12XX_FEATURES=ON -f LA12XX_DRIVER_PATRON_RF=ON

4. Or Build for ICEWINGS RF
/clean.sh; ./build.sh -t la1224 -l info -b debug -m pci -f LA12XX_FEATURES=ON -f LA12XX_DRIVER_ICEWINGS_RF=ON
