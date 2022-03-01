# Makefile for discord_presence plugin
CC=zig cc
CXX=zig c++

TARGET_ARCH?=x86_64


CROSS?=linux

ifeq ($(CROSS),windows)
    TARGET=$(TARGET_ARCH)-windows-gnu
    SUFFIX=dll
else ifeq ($(CROSS),linux)
    TARGET=$(TARGET_ARCH)-linux-gnu
    SUFFIX=so
else ifeq ($(CROSS),macos)
    TARGET=$(TARGET_ARCH)-macos-gnu
    SUFFIX=dylib
else
    $(error unknown cross platform)
endif

STD?=gnu99
CFLAGS+=-fPIC -I /usr/local/include -I discord-rpc/include -I . -Wall
CXXFLAGS+=-fPIC -I /usr/local/include -I . -Wall


ifeq ($(UNAME_S),Darwin)
    CFLAGS+=-I $(DEADBEEF_OSX)/Contents/Headers
    CXXFLAGS+=-I $(DEADBEEF_OSX)/Contents/Headers
endif
ifeq ($(DEBUG),1)
CFLAGS +=-g -O0
CXXFLAGS +=-g -O0
else
CFLAGS += -O2
CXXFLAGS += -O2
endif

PREFIX=/usr/local/lib/deadbeef
ifeq ($(UNAME_S),Darwin)
    PREFIX=$(DEADBEEF_OSX)/Contents/Resources
endif

ARTWORK_OBJS=artwork/artwork_internal.o artwork/escape.o artwork/lastfm.o
PLUGNAME=discord_presence

RPC_DIR :=discord-rpc/src
RPC_FILES := $(wildcard $(RPC_DIR)/*.cpp)

ifeq ($(TARGET),x86_64-windows-gnu)
    RPC_FILES := $(filter-out $(RPC_DIR)/connection_unix.cpp $(RPC_DIR)/discord_register_linux.cpp, $(RPC_FILES))
else
    RPC_FILES := $(filter-out $(RPC_DIR)/connection_win.cpp $(RPC_DIR)/discord_register_win.cpp $(RPC_DIR)/dllmain.cpp, $(RPC_FILES))
endif
RPC_FILES := $(patsubst $(RPC_DIR)/%.cpp,$(RPC_DIR)/%.o,$(RPC_FILES))

all: submodules_load discord_presence

discord_presence: discord-rpc $(ARTWORK_OBJS)
	$(CC)  --target=$(TARGET) -std=$(STD) -c $(CFLAGS) -c $(PLUGNAME).c -o $(PLUGNAME).o
	$(CXX) --target=$(TARGET) -std=$(STD) -shared $(CXXFLAGS) -o $(PLUGNAME).$(SUFFIX) $(PLUGNAME).o $(RPC_FILES) $(ARTWORK_OBJS) $(LIBS) $(CXX_LDFLAGS) $(LDFLAGS)

$(ARTWORK_OBJS):
	$(MAKE) -C artwork/ TARGET=$(TARGET)

discord-rpc: discord-rpc-patch
	$(MAKE) -C discord-rpc/ TARGET=$(TARGET)

submodules_load:
	git submodule init
	git submodule update

discord-rpc-patch:
	@cd discord-rpc; \
	git apply ../00-discord-rpc.patch --check 2>/dev/null >/dev/null; \
	if [ $$? -eq 0 ]; then \
	git apply ../00-discord-rpc.patch;\
	fi

discord-rpc-patch-reverse:
	@cd discord-rpc; \
	git apply ../00-discord-rpc.patch --check --reverse 2>/dev/null >/dev/null; \
	if [ $$? -eq 0 ]; then \
	git apply --reverse ../00-discord-rpc.patch;\
	fi

install:
	cp $(PLUGNAME).$(SUFFIX) $(PREFIX)

clean:
	$(MAKE) -C discord-rpc/ clean
	$(MAKE) -C artwork/ clean
	rm -fv $(PLUGNAME).o

.PHONY: discord-rpc discord-rpc-patch
