#!/bin/sh

if [ "$(uname)" == "Darwin" ]; then
    c++ -std=c++17 -x objective-c++ \
     jo_basic.cpp \
     -Os -fno-exceptions -lpthread -lobjc \
     -framework Cocoa -framework QuartzCore -framework OpenGL \
     -o basic
fi
