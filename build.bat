@echo off
mkdir build
pushd build
g++ -Wall -Wextra -Werror -pedantic -std=c++20 -g ../src/win_hspaceinvaders.cpp -o ../win_hspaceinvaders.exe -lgdi32 -lole32 -L "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.18362.0\um\x64" -l:xaudio2.lib
popd
