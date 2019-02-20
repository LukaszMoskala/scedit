CC=gcc
CXX=g++
#append these flags to command line flags
override CXXFLAGS+=-O3 --std=c++17 -fexceptions
PREFIX=/usr/local

all: scedit

scedit.o: scedit.cpp
	$(CXX) $(CXXFLAGS) -c -o scedit.o scedit.cpp
scedit: scedit.o
	$(CXX) $(CXXFLAGS) -o scedit scedit.o

install:
	install -m 775 scedit $(PREFIX)/bin/

clean:
	rm -f scedit.o
