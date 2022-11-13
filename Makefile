JO_TARGET=basic
DESTDIR=/usr/local/bin

$(JO_TARGET):
	c++ -std=c++17 jo_clojure.cpp -g -O2 -fno-exceptions -lpthread -o $(JO_TARGET)

install: $(JO_TARGET)
	mkdir -p '$(DESTDIR)'
	cp $(JO_TARGET) $(DESTDIR)

clean: $(JO_TARGET)
	rm $(JO_TARGET)
