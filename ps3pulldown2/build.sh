#!/bin/bash

export PICO_SDK_PATH=$PWD/pico_sdk

cd ps3pulldown2

#rm -rf build
mkdir build

cd build

cmake ..

make clean
make ps3pulldown2 -j12
