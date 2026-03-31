# X# (Xsharp) Build System
CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic
LDFLAGS ?= -lm -lpthread
PREFIX  ?= /usr/local

# Source files
CORE_SRC := $(wildcard src/lexer/*.c src/parser/*.c src/ast/*.c \
	src/compiler/*.c src/codegen/*.c src/vm/*.c src/runtime/*.c \
	src/debugger/*.c src/cli/*.c src/formats/*.c \
	src/renderer/core/*.c src/renderer/gpu/*.c \
	src/renderer/shaders/*.c src/renderer/scene/*.c)
STDLIB_SRC := $(wildcard stdlib/*/*.c)
ALL_SRC := src/main.c $(CORE_SRC) $(STDLIB_SRC)
ALL_OBJ := $(ALL_SRC:.c=.o)

TEST_SRC := $(wildcard tests/*.c tests/*/*.c)

# Targets
.PHONY: all release debug test clean install uninstall

all: xsharp

release: CFLAGS += -O2 -DNDEBUG
release: xsharp

debug: CFLAGS += -g -O0 -DDEBUG
debug: xsharp

xsharp: $(ALL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I src/ -I stdlib/ -c -o $@ $<

test: xsharp
	@echo "Running tests..."
	@for f in examples/*.xs; do \
		echo "  Testing $$f..."; \
		./xsharp run "$$f" > /dev/null 2>&1 && echo "    PASS" || echo "    FAIL"; \
	done

clean:
	find . -name '*.o' -delete
	rm -f xsharp xsharp-ide
	rm -rf build/

install: xsharp
	install -d $(PREFIX)/bin
	install -m 755 xsharp $(PREFIX)/bin/
	install -d $(PREFIX)/share/xsharp
	cp -r stdlib $(PREFIX)/share/xsharp/
	cp -r examples $(PREFIX)/share/xsharp/

uninstall:
	rm -f $(PREFIX)/bin/xsharp
	rm -rf $(PREFIX)/share/xsharp
