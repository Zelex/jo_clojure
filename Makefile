JO_TARGET=jclj
DESTDIR=/usr/local/bin

$(JO_TARGET):
	c++ -std=c++17 -Iext -DNO_SOKOL -DNO_MYSQL -DNO_NFD jo_clojure.cpp -Os -fexceptions -lpthread -o $(JO_TARGET)

$(JO_TARGET)_debug:
	c++ -std=c++17 -Iext -DNO_SOKOL -DNO_MYSQL -DNO_NFD jo_clojure.cpp -g -O0 -fexceptions -lpthread -o $(JO_TARGET)_debug

all: $(JO_TARGET) $(JO_TARGET)_debug

debug: $(JO_TARGET)_debug

install: $(JO_TARGET)
	mkdir -p '$(DESTDIR)'
	cp $(JO_TARGET) $(DESTDIR)

clean: 
	rm $(JO_TARGET) $(JO_TARGET)_debug
