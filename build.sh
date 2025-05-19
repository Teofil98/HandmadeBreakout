#!/bin/bash

mkdir -p build
pushd build
g++ -Wall -Wextra -Werror -pedantic -fsanitize=address -fsanitize=undefined -std=c++20 -g ../src/linux_hspaceinvaders.cpp \
                            ../src/hspaceinvaders.cpp \
                            ../src/entities.cpp \
                            -lX11 -lpulse-simple -lpulse -lpthread \
                            -o ../linux_hspaceinvaders.elf
