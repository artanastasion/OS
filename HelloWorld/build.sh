#!/bin/bash
git pull
mkdir -p build
cd build
cmake ..
make
