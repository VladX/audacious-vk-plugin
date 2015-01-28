CXX = g++
CXXFLAGS = `pkg-config --cflags gtk+-3.0 libsoup-2.4 audacious` -O2 -g -Wall -Wextra -fPIC -std=c++11
LDFLAGS = -shared
AUD_PLUGINS_DIR = `pkg-config --variable=general_plugin_dir audacious`
LIBS = `pkg-config --libs gtk+-3.0 libsoup-2.4 audacious`
LIBNAME = vk-plugin.so

SOURCES = vk-plugin.cpp json11.cpp https-request.cpp

TEST_SOURCES = test.cpp
TEST_CXXFLAGS = `pkg-config --cflags gtk+-3.0 libsoup-2.4 audacious` -O2 -g -Wall -Wextra -std=c++11

all: $(LIBNAME) test

clean:
	rm -f *.o *.so test

$(LIBNAME):
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SOURCES) $(LIBS) -o $(LIBNAME)

test:
	$(CXX) $(TEST_CXXFLAGS) $(TEST_SOURCES) $(LIBNAME) $(LIBS) -o test

install:
	mkdir -p $(AUD_PLUGINS_DIR)
	cp $(LIBNAME) $(AUD_PLUGINS_DIR)
	
uninstall:
	rm $(AUD_PLUGINS_DIR)/$(LIBNAME)