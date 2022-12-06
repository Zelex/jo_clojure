JO_TARGET=basic
DESTDIR=/usr/local/bin

$(JO_TARGET):
	c++ -std=c++17 -DNO_SOKOL jo_basic.cpp -Os -fno-exceptions -lpthread -o $(JO_TARGET)

$(JO_TARGET)_debug:
	c++ -std=c++17 -DNO_SOKOL jo_basic.cpp -g -O0 -fno-exceptions -lpthread -o $(JO_TARGET)_debug

all: $(JO_TARGET) $(JO_TARGET)_debug

debug: $(JO_TARGET)_debug

install: $(JO_TARGET)
	mkdir -p '$(DESTDIR)'
	cp $(JO_TARGET) $(DESTDIR)

clean: 
	rm $(JO_TARGET) $(JO_TARGET)_debug
