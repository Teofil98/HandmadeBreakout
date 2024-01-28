#!/bin/bash

mkdir -p build
pushd build
g++ -Wall -Wextra -Werror -pedantic -std=c++20 -g ../src/linux_hspaceinvaders.cpp \
                    -lX11 -o ../linux_hspaceinvaders.elf 
