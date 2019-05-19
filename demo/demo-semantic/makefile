SHELL = /bin/bash

SRC = $(wildcard src/*.alioth)
INC = $(wildcard inc/*.alioth)

all: demo-semantic

demo-semantic: $(SRC) $(INC)
	aliothc : demo-semantic

run: all
	./bin/demo-semantic

clean:
	rm -rf bin/* obj/*.o

.PHONY: clean
