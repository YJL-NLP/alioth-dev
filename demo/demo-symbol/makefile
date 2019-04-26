SHELL = /bin/bash

SRC = $(wildcard src/*.alioth)
INC = $(wildcard inc/*.alioth)

all: demo-symbol

demo-symbol: $(SRC) $(INC)
	aliothc : demo-symbol

run: all
	./bin/demo-symbol

clean:
	rm -rf bin/* obj/*.o

.PHONY: clean
