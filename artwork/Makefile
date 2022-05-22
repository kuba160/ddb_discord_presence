# SPDX-FileCopyrightText: 2022 Jakub Wasylków <kuba_160@protonmail.com>
# SPDX-License-Identifier: CC0-1.0
CC?=gcc
STD?=gnu99
CFLAGS+=-fPIC -I /usr/local/include -I .. -Wall -g
OBJ = artwork_internal.o escape.o lastfm.o

%.o: %.c $(DEPS)
	$(CC) -std=$(STD) -c -o $@ $< $(CFLAGS)

all: $(OBJ)

clean:
	rm -v $(OBJ)
