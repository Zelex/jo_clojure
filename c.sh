#!/bin/bash

# https://stackoverflow.com/questions/3466166/how-to-check-if-running-in-cygwin-mac-or-linux/27776822

if [ "$(uname)" == "Darwin" ]; then
    c++ -std=c++17 -x objective-c++ \
     -Iext -I/opt/homebrew/opt/mysql-client/include \
     jo_basic.cpp \
     imgui/imgui.cpp imgui/imgui_widgets.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_demo.cpp \
     -O2 -fexceptions -lpthread -lobjc \
     -framework Cocoa -framework QuartzCore -framework OpenGL -framework Metal -framework MetalKit \
     -L/opt/homebrew/opt/mysql-client/lib \
     -lmysqlclient \
     -o basic
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    c++ -std=c++17 \
     -DNO_SOKOL \
     -Iext \
     jo_basic.cpp \
     imgui/imgui.cpp imgui/imgui_widgets.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_demo.cpp \
     -O2 -fexceptions -lpthread \
     -lmysqlclient \
     -o basic
fi
