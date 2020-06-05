SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(patsubst src/%,build/%,${SOURCES:.cpp=.o})

CXX=g++
LD_FLAGS=
CXX_FLAGS=-std=c++17

NAME=chip8
BINARY=out/${NAME}

all: dirs ${BINARY}

${BINARY}: ${OBJECTS}
	${CXX} $^ ${LD_FLAGS} -o $@

build/%.o: src/%.cpp
	${CXX} $< ${CXX_FLAGS} -c -o $@

.PHONY: dirs
dirs:
	mkdir -p build/
	mkdir -p out/

.PHONY: clean
clean:
	-rm -r build/
	-rm -r out/
