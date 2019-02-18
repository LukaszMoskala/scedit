CC=gcc
CXX=g++
CXXFLAGS=-O3 --std=c++17 -fexceptions
CFLAGS=-O3
PREFIX=/usr/local

all: scedit

scedit.o: scedit.cpp
	$(CXX) $(CXXFLAGS) -c -o scedit.o scedit.cpp
scedit: scedit.o
	$(CXX) $(CXXFLAGS) -o  scedit  scedit.o

install:
	install -m 775 scedit $(PREFIX)/bin/
