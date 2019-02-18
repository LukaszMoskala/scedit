CC=gcc
CXX=g++
#why static? because I need to copy binary to my school computer
#and expect it to work right away. I will compile it there if needed
#but linking statically is better for me
CXXFLAGS=-O3 --std=c++17 -fexceptions --static -static-libgcc
PREFIX=/usr/local

all: warning scedit

warning:
	@echo 'This is NOT meant for production usage!'
	@echo 'And it takes 8.454 seconds to compile on'
	@echo 'i5-4670k@4.8GHz. This is GARBAGE, DONT'
	@echo 'use this program if you dont need to!'

scedit.o: scedit.cpp
	$(CXX) $(CXXFLAGS) -c -o scedit.o scedit.cpp
scedit: scedit.o
	$(CXX) $(CXXFLAGS) -o scedit scedit.o

install:
	install -m 775 scedit $(PREFIX)/bin/

clean:
	rm -f scedit.o
