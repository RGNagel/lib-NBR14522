#/usr/bin/bash

#
# CHANGE TC_TARGET_ENV_PATH AND TC_TARGET_BIN_PATH VAR. TO REFLECT THE PLACE 
# WHERE THE TOOLCHAIN IS LOCATED. ALSO, YOU MAY NEED TO CHANGE OTHER VARIABLES
# IN TC-raspberry.cmake FILE ACCORDING TO YOUR TARGET OR NEEDS.
#
# To use this toolchain, please follow the following installation for the 
# raspberry toolchain: https://github.com/Pro/raspi-toolchain
#
# It must be executed in repository's root directory:
# ./scripts/raspberry/build.sh
#

TC_ENV_PATH=/opt/cross-pi-gcc
TC_BIN_PATH=/opt/cross-pi-gcc/bin/arm-linux-gnueabihf

mkdir build-raspberry
cd build-raspberry
cmake \
    -DTC_BIN_PATH=${TC_BIN_PATH} \
    -DTC_ENV_PATH=${TC_ENV_PATH} \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/TC-raspberry.cmake .. \

make
cd -

