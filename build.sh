#!/bin/sh

cmake -G "CodeLite - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build --config Debug
