# Makefile for discord_presence plugin
ifeq ($(OS),Windows_NT)
    SUFFIX = dll
else
    SUFFIX = so
endif

CC=gcc
CXX=g++
STD=gnu99
CFLAGS=-fPIC -I /usr/local/include -Wall -I ./include
CXXFLAGS=-fPIC -I /usr/local/include -Wall
ifeq ($(DEBUG),1)
CFLAGS +=-g -O0
CXXFLAGS +=-g -O0
endif

PREFIX=/usr/local/lib/deadbeef
PLUGNAME=discord_presence
LIBS=libdiscord-rpc.a

all:
	$(CC) -std=$(STD) -c $(CFLAGS) -c $(PLUGNAME).c
	$(CXX) -std=$(STD) -shared $(CXXFLAGS) -o $(PLUGNAME).$(SUFFIX) $(PLUGNAME).o $(LIBS)

install:
	cp $(PLUGNAME).$(SUFFIX) $(PREFIX)

clean:
	rm -fv $(PLUGNAME).o $(PLUGNAME).$(SUFFIX)