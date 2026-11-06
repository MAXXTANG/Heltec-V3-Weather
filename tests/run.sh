#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/countdown-frames build/alert-frames
cxx="${CXX:-clang++}"
for test in render schedule battery countdown disaster setup disaster_events disaster_simulation; do
    "$cxx" -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g "tests/$test.cpp" -o "build/$test"
done
build/render build/clock.pbm build/weather.pbm build
build/schedule
build/battery
build/countdown build/countdown-frames
build/disaster
build/disaster_events
build/disaster_simulation
build/setup
python3 tools/render_setup_preview.py
python3 tools/render_disaster_preview.py
python3 tests/runtime_contract.py
python3 tools/render_previews.py
python3 tools/render_countdown_preview.py
