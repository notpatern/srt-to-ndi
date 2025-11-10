#!/bin/bash

mkdir build

cmake --preset=debug
mv ./build/compile_commands.json ./compile_commands.json

mingw32-make -C ./build

PAUSE
