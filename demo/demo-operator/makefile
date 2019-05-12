SHELL = /bin/bash

SRC = $(wildcard src/*.alioth)
INC = $(wildcard inc/*.alioth)

all: demo-operator

demo-operator: $(SRC) $(INC)
	aliothc : demo-operator

run: all
	./bin/demo-operator

clean:
	rm -rf bin/* obj/*.o

.PHONY: clean
