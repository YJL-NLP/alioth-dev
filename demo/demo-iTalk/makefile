SHELL = /bin/bash

SRC = $(wildcard src/*.alioth)
INC = $(wildcard inc/*.alioth)

all: $(SRC) $(INC)
	aliothc : demo-iTalk

run: all
	./bin/demo-iTalk

clean:
	rm -rf bin/* obj/*.o

.PHONY: clean
