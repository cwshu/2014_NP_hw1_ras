CC = gcc
CFLAGS = -std=c99 -g
CXX = clang++
CXXFLAGS = -std=c++11 -g

EXE = ras
OBJS = ras.o socket.o io_wrapper.o parser.o cstring_more.o pipe_manager.o server_arch.o

MAKE = make

# platform issue
UNAME = $(shell uname)
ifeq ($(UNAME), FreeBSD)
    MAKE = gmake
endif

all: ${EXE}

clean: 
	rm -f ${EXE} ${OBJS}

${EXE}: ${OBJS}
	${CXX} -o $@ ${CXXFLAGS} $^

$(OBJS): %.o: %.cpp
	${CXX} -o $@ ${CXXFLAGS} -c $<

# build TA testing environment
TA_test:
	$(MAKE) all install -C $@

.PHONY: all clean TA_test
