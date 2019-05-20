SHELL = /bin/bash

SRC = $(wildcard src/*.alioth)
INC = $(wildcard inc/*.alioth)

all: demo-as

demo-as: $(SRC) $(INC)
	aliothc : demo-as

run: all
	./bin/demo-as

clean:
	rm -rf bin/* obj/*.o

.PHONY: clean
