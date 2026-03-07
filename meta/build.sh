#!/bin/bash

# Set the library path for runtime
LIB_PATH="/opt/p2996/clang/lib/x86_64-unknown-linux-gnu"

# Compile the reflection.cpp file with the required flags
clang++ -std=c++26 -freflection reflection.cpp -o ref -Wl,-rpath=$LIB_PATH

./ref
