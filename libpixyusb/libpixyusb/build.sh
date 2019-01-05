#!/bin/sh

BUILD_ROOT=`pwd`

sudo apt-add-repository ppa:wpilib/toolchain
sudo apt update
sudo apt install frc-toolchain

cd "$BUILD_ROOT/build/"
wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.bz2
wget https://github.com/libusb/libusb/releases/download/v1.0.21/libusb-1.0.21.tar.bz2
tar xvfj boost_1_65_1.tar.bz2
tar xvfj libusb-1.0.21.tar.bz2

cd "$BUILD_ROOT/build/boost_1_65_1/tools/build/"
./bootstrap.sh
cd "$BUILD_ROOT/build/boost_1_65_1/"
echo 'using gcc : : arm-frc-linux-gnueabi-g++ ;' > ./user-config.jam
./b2 install --prefix="$BUILD_ROOT/build/boost_1_65_1_build/"
export PATH="$BUILD_ROOT/build/boost_1_65_1_build/bin:$PATH"
b2 toolset=gcc link=static cflags=-fPIC cxxflags=-fPIC stage

cd "$BUILD_ROOT/build/libusb-1.0.21/"
CROSS_COMPILE=arm-frc-linux-gnueabi- CC=arm-frc-linux-gnueabi-gcc CXX=arm-frc-linux-gnueabi-g++ CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --host=arm-linux --enable-udev=no --prefix="$BUILD_ROOT/build/libusb-1.0.21_out"
make
make install

cd "$BUILD_ROOT/src/host/libpixyusb"
cmake -DBoost_NO_SYSTEM_PATHS=ON -DBoost_LIBRARY_DIRS="$BUILD_ROOT/build/boost_1_65_1/stage/lib" -DBoost_INCLUDE_DIR="$BUILD_ROOT/build/boost_1_65_1" -DLIBUSB_1_INCLUDE_DIRS="$BUILD_ROOT/build/libusb-1.0.21_out/include/libusb-1.0" -DLIBUSB_1_LIBRARIES="$BUILD_ROOT/build/libusb-1.0.21_out/lib/libusb-1.0.a" -DCMAKE_C_COMPILER=arm-frc-linux-gnueabi-gcc -DCMAKE_CXX_COMPILER=arm-frc-linux-gnueabi-g++ -DCMAKE_INSTALL_PREFIX="$BUILD_ROOT/out"
make
make install
