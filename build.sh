#!/bin/bash

mkdir -p build
pushd build
g++ -Wall -Wextra -Werror -pedantic -std=c++20 -g ../src/linux_hspaceinvaders.cpp \
						  ../src/hspaceinvaders.cpp \
                    -lX11 -lasound -o ../linux_hspaceinvaders.elf 
