@echo off
mkdir build
pushd build
g++ -Wall -Wextra -Werror -g ../src/win_hspaceinvaders.cpp -o ../win_hspaceinvaders.exe
popd
