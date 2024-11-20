# SPDX-FileCopyrightText: 2022 Jakub Wasylków <kuba_160@protonmail.com>
# SPDX-License-Identifier: CC0-1.0

# Makefile for discord_presence plugin
ifeq ($(OS),Windows_NT)
    SUFFIX ?= dll
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        SUFFIX ?= dylib
        DEADBEEF_OSX = /Applications/DeaDBeeF.app
    else
        SUFFIX ?= so
    endif
endif

CC?=gcc
CXX?=g++
STD?=gnu99
CFLAGS+=$(COPT) -fPIC -I /usr/local/include -I discord-rpc/include -I . -Wall
CXXFLAGS+=$(CXXOPT) -fPIC -I /usr/local/include -I . -Wall
ifeq ($(UNAME_S),Darwin)
    CFLAGS+=-I $(DEADBEEF_OSX)/Contents/Headers
    CXXFLAGS+=-I $(DEADBEEF_OSX)/Contents/Headers
endif
ifeq ($(DEBUG),1)
CFLAGS +=-g -O0
CXXFLAGS +=-g -O0
endif

PREFIX=/usr/local/lib/deadbeef
ifeq ($(UNAME_S),Darwin)
    PREFIX=$(DEADBEEF_OSX)/Contents/Resources
endif
PLUGNAME=discord_presence
LIBS=libdiscord-rpc.a -lpthread
ARTWORK_OBJS=artwork/artwork_internal.o artwork/escape.o artwork/lastfm.o

all: libdiscord-rpc.a discord_presence

discord_presence: libdiscord-rpc.a
	CFLAGS="$(CFLAGS)" $(MAKE) -C artwork
	$(CC) -std=$(STD) -c $(CFLAGS) -c $(PLUGNAME).c
	$(CXX) -std=$(STD) -shared $(CXXFLAGS) -o $(PLUGNAME).$(SUFFIX) $(PLUGNAME).o $(ARTWORK_OBJS) $(LIBS) $(CXX_LDFLAGS) $(LDFLAGS)

libdiscord-rpc.a:
	cd discord-rpc && CFLAGS="$(CFLAGS)" $(MAKE)
	cp discord-rpc/src/libdiscord-rpc.a .

install:
	cp $(PLUGNAME).$(SUFFIX) $(PREFIX)

clean:
	cd discord-rpc && git clean -df && git reset --hard
	rm -fv $(PLUGNAME).o $(PLUGNAME).$(SUFFIX) $(ARTWORK_OBJS) libdiscord-rpc.a
