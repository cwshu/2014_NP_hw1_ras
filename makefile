CC = gcc
CFLAGS = -std=c99 -g
CXX = clang++
CXXFLAGS = -std=c++11 -g

EXE = ras
OBJS = ras.o socket.o io_wrapper.o parser.o cstring_more.o pipe_manager.o server_arch.o
EXE2 = ras_single_process
OBJS2 = ras_single_process.o socket.o io_wrapper.o parser.o cstring_more.o pipe_manager.o server_arch.o number_pool.o

# union
OBJS1_2 = $(sort $(OBJS) $(OBJS2))

MAKE = make

# platform issue
UNAME = $(shell uname)
ifeq ($(UNAME), FreeBSD)
    MAKE = gmake
endif

all: $(EXE) $(EXE2)

clean: 
	rm -f $(EXE) $(OBJS) $(EXE2) $(OBJS2)

$(EXE): $(OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $^

$(EXE2): $(OBJS2)
	$(CXX) -o $@ $(CXXFLAGS) $^

$(OBJS1_2): %.o: %.cpp
	$(CXX) -o $@ $(CXXFLAGS) -c $<

# build TA testing environment
TA_test:
	$(MAKE) clean all install -C $@

.PHONY: all clean TA_test
