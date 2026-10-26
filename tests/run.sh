#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cxx="${CXX:-clang++}"
for test in render schedule battery; do
    "$cxx" -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g "tests/$test.cpp" -o "build/$test"
done
build/render build/clock.pbm build/weather.pbm build
build/schedule
build/battery
python3 tests/runtime_contract.py
python3 tools/render_previews.py
