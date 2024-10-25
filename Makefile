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

all: submodules_load libdiscord-rpc.a discord_presence

discord_presence:
	$(MAKE) -C artwork
	$(CC) -std=$(STD) -c $(CFLAGS) -c $(PLUGNAME).c
	$(CXX) -std=$(STD) -shared $(CXXFLAGS) -o $(PLUGNAME).$(SUFFIX) $(PLUGNAME).o $(ARTWORK_OBJS) $(LIBS) $(CXX_LDFLAGS) $(LDFLAGS)

libdiscord-rpc.a: discord-rpc-patch
	cd discord-rpc && $(MAKE)
	cp discord-rpc/src/libdiscord-rpc.a .

submodules_load:
	git submodule init
	git submodule update

discord-rpc-patch:
	@cd discord-rpc; \
	git apply ../00-discord-rpc.patch --check 2>/dev/null >/dev/null; \
	if [ $$? -eq 0 ]; then \
	git apply ../00-discord-rpc.patch;\
	fi
	@cd discord-rpc; \
	git apply ../02-activity_type.patch --check 2>/dev/null >/dev/null; \
	if [ $$? -eq 0 ]; then \
	git apply ../02-activity_type.patch;\
	fi

discord-rpc-patch-reverse:
	@cd discord-rpc; \
	git apply ../00-discord-rpc.patch --check --reverse 2>/dev/null >/dev/null; \
	if [ $$? -eq 0 ]; then \
	git apply --reverse ../00-discord-rpc.patch;\
	fi
	@cd discord-rpc; \
	git apply ../02-activity_type.patch --check --reverse 2>/dev/null >/dev/null; \
	if [ $$? -eq 0 ]; then \
	git apply --reverse ../02-activity_type.patch;\
	fi

install:
	cp $(PLUGNAME).$(SUFFIX) $(PREFIX)

clean: discord-rpc-patch-reverse
	cd discord-rpc && git clean -df && git reset --hard
	rm -fv $(PLUGNAME).o $(PLUGNAME).$(SUFFIX)
