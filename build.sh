#!/bin/bash

usage() {
    echo "Usage: $0 [release/debug]" >&2
    exit
}

if [ $# -ne  1 ]; then
    echo "Error! Wrong number of arguments." >&2
    usage
fi

if [ $1 = "release" ]; then
    flags="-O3"
    link_flags=""
elif [ $1 = "debug" ]; then
    flags="-Wall -Wextra -Werror -pedantic -fsanitize=address -fsanitize=undefined"
    link_flags=""
else
    echo "Error! Unknown argument \"$1\". Expected \"debug\" or \"release\"" >&2
    usage
fi

mkdir -p build
pushd build
gcc $flags -std=c99 -g ../src/engine_core/linux_engine_core.c \
                            ../src/game.c \
                            ../src/ECS/entities.c \
                            -lX11 -lpulse-simple -lpulse -lpthread \
                            $link_flags \
                            -o ../linux_game.elf

# strip -s -R .comment -R .gnu.version --strip-unneeded
