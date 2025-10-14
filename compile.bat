#!/bin/bash

mkdir build
cd build

cmake -S .. -B .
mv compile_commands.json ../compile_commands.json

cmake --build .

PAUSE
