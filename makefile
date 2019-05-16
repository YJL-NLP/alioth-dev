SHELL = /bin/bash

SRC = $(wildcard src/*.cpp)
HED = $(wildcard inc/*.hpp)
ASC = $(wildcard app/*.cpp)
RTS = $(wildcard src/*.asm)

RTG = $(RTS:src/%.asm=root/obj/%.o)
ATG = $(ASC:app/%.cpp=bin/%)
OBJ = $(SRC:src/%.cpp=obj/%.o)

LLVMOOPT = $(shell llvm-config --cxxflags)
LLVMLOPT = $(shell llvm-config --ldflags --system-libs --link-static --libs x86codegen)

COMOPT = -std=gnu++17 -g -O0
OOPT = $(LLVMOOPT) $(COMOPT)
BOPT = $(LLVMOOPT) $(LLVMLOPT) $(COMOPT)

all: $(OBJ) $(RTG) $(ATG)

clean:
	rm -rf obj/*

install: all
	sudo cp bin/aliothc /usr/bin/aliothc
	sudo cp -r root/. /usr/lib/alioth
	sudo cp root/inc/aliothc /usr/share/bash-completion/completions/

initial:
	if ! [ -d bin ]; then mkdir bin; fi
	if ! [ -d obj ]; then mkdir obj; fi
	if ! [ -d obj/web ]; then mkdir obj/web; fi
	if ! [ -d root/obj ]; then mkdir root/obj; fi
	sudo cp -r root/. /usr/lib/alioth

.PHONY: clean install initial

$(OBJ):obj/%.o:src/%.cpp $(HED) # makefile
	g++-8 $(OOPT) -Iinc -c $< -o $@

$(ATG):bin/%:app/%.cpp $(OBJ) # makefile
	g++-8 -Iinc $< $(OBJ) $(BOPT) -o $@

$(RTG):root/obj/%.o:src/%.asm # makefile
	nasm -f elf64 $< -o $@