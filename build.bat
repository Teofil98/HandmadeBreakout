@echo off
mkdir build
pushd build
g++ -g ../win_hspaceinvaders.cpp -o win_hspaceinvaders.exe
popd
