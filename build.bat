@echo off
mkdir build
pushd build
gcc -Wall -Wextra -Werror -std=c99 -g ^
                    ../src/engine_core/win32_engine_core.c ^
                    ../src/game.c ^
                    ../src/ECS/entities.c ^
                    -o ../win32_hspaceinvaders.exe ^
                    -lgdi32 -lole32 -lxaudio2_9
popd
