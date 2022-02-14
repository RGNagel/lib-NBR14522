# IMPORTANT:
# To use this toolchain, please follow the following installation for the 
# raspberry toolchain: https://github.com/Pro/raspi-toolchain

# to understand the meaning of the parameters in this file, please read:
# https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html

# how to use:
# mkdir build && cd build
# cmake -DTC_BIN_PATH=/env/bin/path -DTC_ENV_PATH=/env/ -DCMAKE_TOOLCHAIN_FILE=../cmake/TC-raspberry.cmake ..
# make


# the name of the target operating system
set(CMAKE_SYSTEM_NAME Raspberrypi)
set(CMAKE_SYSTEM_VERSION 5.10.17)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   ${TC_BIN_PATH}-gcc)
set(CMAKE_CXX_COMPILER ${TC_BIN_PATH}-g++)

# where is the target environment located
set(CMAKE_FIND_ROOT_PATH  ${TC_ENV_PATH})

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment only
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
