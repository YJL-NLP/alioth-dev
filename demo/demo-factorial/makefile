SHELL = /bin/bash

SRC = $(wildcard src/*.alioth)
INC = $(wildcard inc/*.alioth)

all: $(SRC) $(INC)
	aliothc : demo-factorial

run: all
	./bin/demo-factorial

clean:
	rm -rf bin/* obj/*.o

.PHONY: clean
