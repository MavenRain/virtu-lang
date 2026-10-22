# Builds the tcc-json kit with TinyCC. See README.md.
TCC ?= tcc
CC ?= cc
CFLAGS = -std=c99 -Wall -Werror -Isrc
CLANG_FLAGS = -std=c99 -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only -Isrc
FRONT = src/front/base.c src/front/lexer.c src/front/parser.c src/front/front.c src/front/eval.c src/front/check.c src/front/big.c
TARGET = src/json.c
SRC = src/main.c $(FRONT) $(TARGET)
HEADERS = $(wildcard src/*.h src/front/*.h)

.PHONY: build check check-clang clean

build: build/langc

build/domain.c: domain/domain.lang gen/embed.c
	mkdir -p build
	$(TCC) -run gen/embed.c domain/domain.lang build/domain.c

build/langc: $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/langc $(SRC) build/domain.c

build/path-regressions: test/path-regressions.c $(FRONT) $(TARGET) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o $@ test/path-regressions.c $(FRONT) $(TARGET) build/domain.c

check-clang: build/domain.c
	$(CC) $(CLANG_FLAGS) $(SRC) test/path-regressions.c build/domain.c

check: build/langc build/path-regressions check-clang
	sh test/gate.sh
	TCC="$(TCC)" node test/regressions.js
	node test/printer-regressions.js
	build/path-regressions

clean:
	rm -rf build
