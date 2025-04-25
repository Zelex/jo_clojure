JO_TARGET=jclj
DESTDIR=/usr/local/bin
SRCS=jo_clojure.cpp 
OBJS=$(SRCS:.cpp=.o)
CXX=c++
CXXFLAGS=-std=c++17 -Iext -DNO_SOKOL -DNO_MYSQL -DNO_NFD
LDFLAGS=-lpthread

# Production build
$(JO_TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -Os -fexceptions -o $@

# Debug build
$(JO_TARGET)_debug: CXXFLAGS += -g -O0
$(JO_TARGET)_debug: $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -fexceptions -o $@

# Object file rule
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -O3 -fexceptions -c $< -o $@

# Generate dependencies automatically
DEPS=$(SRCS:.cpp=.d)
-include $(DEPS)
%.d: %.cpp
	@$(CXX) $(CXXFLAGS) -MM -MP -MT '$*.o $*.d' $< > $@

all: $(JO_TARGET) $(JO_TARGET)_debug

debug: $(JO_TARGET)_debug

install: $(JO_TARGET)
	mkdir -p '$(DESTDIR)'
	cp $(JO_TARGET) $(DESTDIR)

clean: 
	rm -f $(JO_TARGET) $(JO_TARGET)_debug $(OBJS) $(DEPS)

.PHONY: all debug install clean
