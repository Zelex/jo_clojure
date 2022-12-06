#!/bin/sh

# https://stackoverflow.com/questions/3466166/how-to-check-if-running-in-cygwin-mac-or-linux/27776822

if [ "$(uname)" == "Darwin" ]; then
    c++ -std=c++17 -x objective-c++ \
     jo_basic.cpp \
     imgui/imgui.cpp imgui/imgui_widgets.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_demo.cpp \
     -Os -fno-exceptions -lpthread -lobjc \
     -framework Cocoa -framework QuartzCore -framework OpenGL -framework Metal -framework MetalKit \
     -o basic
fi
