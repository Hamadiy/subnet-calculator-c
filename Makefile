CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2
BIN      := bin

all: $(BIN)/ipcalc

$(BIN)/ipcalc: src/main.c src/ipv4.c src/ipv4.h | $(BIN)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/main.c src/ipv4.c

$(BIN)/test_ipv4: tests/test_ipv4.c src/ipv4.c src/ipv4.h | $(BIN)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_ipv4.c src/ipv4.c

$(BIN):
	mkdir -p $@

test: $(BIN)/test_ipv4 $(BIN)/ipcalc
	./$(BIN)/test_ipv4
	./$(BIN)/ipcalc -v 192.168.0.0/24 50 20 10 2

clean:
	rm -rf $(BIN)

.PHONY: all test clean
