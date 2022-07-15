#!/bin/bash
[[ -d build ]] || mkdir -p build
cd build
cmake ..
make npdrv