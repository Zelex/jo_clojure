JO_TARGET=jo
DESTDIR=/usr/local/bin

$(JO_TARGET):
	c++ -std=c++17 jo_lisp.cpp -g -O0 -o $(JO_TARGET)

install: $(JO_TARGET)
	mkdir -p '$(DESTDIR)'
	cp $(JO_TARGET) $(DESTDIR)

clean: $(JO_TARGET)
	rm $(JO_TARGET)
