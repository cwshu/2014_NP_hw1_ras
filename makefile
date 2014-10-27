CC = gcc
CFLAGS = -std=c99 -g
CXX = clang++
CXXFLAGS = -std=c++11 -g

EXE = ras
OBJS = ras.o socket.o

all: ${EXE}

clean: 
	rm -f ${EXE} ${OBJS}

${EXE}: ${OBJS}
	${CXX} -o $@ ${CXXFLAGS} $^

%.o: %.cpp
	${CXX} -o $@ ${CXXFLAGS} -c $<

.PHONY: all clean
