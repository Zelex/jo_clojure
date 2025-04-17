#!/bin/bash

# https://stackoverflow.com/questions/3466166/how-to-check-if-running-in-cygwin-mac-or-linux/27776822

# if argument is debug
if [ "$1" == "debug" ]; then
    opt="-O0 -g"
else
    opt="-O2"
fi

if [ "$(uname)" == "Darwin" ]; then
    c++ -std=c++17 -x objective-c++ \
     -Iext -I/opt/homebrew/opt/mysql-client/include \
     jo_clojure.cpp \
     imgui/imgui.cpp imgui/imgui_widgets.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_demo.cpp \
     $opt -fexceptions -lpthread -lobjc \
     -framework Cocoa -framework QuartzCore -framework OpenGL -framework Metal -framework MetalKit \
     -L/opt/homebrew/opt/mysql-client/lib \
     -lmysqlclient \
     -o jo_clojure
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    c++ -std=c++17 \
     -DNO_SOKOL \
     -Iext -I/usr/include/gtk-3.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/cairo \
     -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/atk-1.0 \
     jo_clojure.cpp \
     imgui/imgui.cpp imgui/imgui_widgets.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_demo.cpp \
     $opt -fexceptions -lpthread \
     -lmysqlclient -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgio-2.0 -lgobject-2.0 -lglib-2.0 \
     -o jo_clojure
fi
