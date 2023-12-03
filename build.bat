@echo off
mkdir build
pushd build
g++ -Wall -Wextra -Werror -g ../win_hspaceinvaders.cpp -o win_hspaceinvaders.exe
popd
