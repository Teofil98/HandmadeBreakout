@echo off
mkdir build
pushd build
g++ -Wall -Wextra -Werror -pedantic -std=c++20 -g ^
                    ../src/win32_hspaceinvaders.cpp ^
                    ../src/hspaceinvaders.cpp ^
                    ../src/entities.cpp ^
                    -o ../win32_hspaceinvaders.exe ^
                    -lgdi32 -lole32 -lxaudio2_9
popd
