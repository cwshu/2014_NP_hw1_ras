CXX = g++
CXXFLAGS =

RAS_DIR = ~/ras
RAS_BIN_DIR = $(RAS_DIR)/bin
BIN_DIR = bin

BUILD_SRC = $(wildcard *.cpp)
BUILD_EXES_NAME = $(patsubst %.cpp,%,$(BUILD_SRC))
BUILD_EXES = $(addprefix $(BIN_DIR)/,$(BUILD_EXES_NAME))
CP_EXES_NAME = ls cat
CP_EXES = $(addprefix $(BIN_DIR)/,$(CP_EXES_NAME))

all: $(BUILD_EXES) $(CP_EXES)
# make all the binary into $(BIN_DIR)
# $(CP_EXES): copy from system binary
# $(BUILD_EXES): build from this directory

install: | $(RAS_BIN_DIR)
# copy all the binary into $(RAS_BIN_DIR)
	cp $(BIN_DIR)/* $(RAS_BIN_DIR)


$(BUILD_EXES): $(BIN_DIR)/%: %.cpp
	$(CXX) -o $@ $(CXXFLAGS) $<

$(CP_EXES): $(BIN_DIR)/%: 
	cp `which $*` $(BIN_DIR)/

$(BUILD_EXES) $(CP_EXES): | $(BIN_DIR) 
# create $(BIN_DIR) before build/copy

$(RAS_BIN_DIR) $(BIN_DIR): 
# create directory
	mkdir -p $@

clean:
	rm -f $(BUILD_EXES) $(CP_EXES)
