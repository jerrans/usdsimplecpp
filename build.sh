#!/bin/sh
mkdir -p build
python ./submodules/usd/build_scripts/build_usd.py ./build
cd build
cmake ../
make -j`nproc`