@echo off

:: Check number of arguments
if "%1"=="" (
    echo Error! Wrong number of arguments.
    goto usage
)

if "%1"=="release" (
    set "flags=-O3"
    set "link_flags="
) else if "%1"=="debug" (
    set "flags=-Wall -Wextra -Werror -fsanitize=address -fsanitize=undefined"
    set "link_flags="
) else (
    echo Error! Unknown argument "%1". Expected "debug" or "release"
    goto usage
)

:: Make sure build directory exists
if not exist build (
    mkdir build
)

pushd build

gcc %flags% -std=c99 -g ^
                    ../src/engine_core/win32_engine_core.c ^
                    ../src/game.c ^
                    ../src/ECS/entities.c ^
                    ../src/ECS/ecs_engine.c ^
                    -lgdi32 -lole32 -lxaudio2_9 ^
                    %link_flags% ^
                    -o ../win32_hspaceinvaders.exe

popd
exit /b 0

:: FUNCTIONS
:usage
echo Usage: %~nx0 [release/debug]
exit /b 1
