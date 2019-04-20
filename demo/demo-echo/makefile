SHELL = /bin/bash

SRC = $(wildcard src/*.alioth)
INC = $(wildcard inc/*.alioth)

all: $(SRC) $(INC)
	aliothc : demo-echo

run: all
	./bin/demo-echo

clean:
	rm -rf bin/* obj/*.o

.PHONY: clean
